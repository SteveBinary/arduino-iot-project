import sys
import time

import paho.mqtt.client as mqtt

from common.configuration import *
from common.sensor_data import sensor_data_from_json_string, SensorData
from database.database import DatabaseManager

CONFIG_FILE_PATH = "config.json"

SENSOR_DATA_BUFFER: list[SensorData] = [] * 1000
CONNECTED_TO_BROKER = False

mqtt.Client.bad_connection_flag = False


def on_connect(client: mqtt.Client, config: Config, rc: int):
    if rc == 0:
        client.connected_flag = True
        logging.info("MQTT client connected!")
        client.subscribe(config.mqtt.topic)
    else:
        client.bad_connection_flag = True
        logging.error(f"MQTT bad connection returned code = {rc}")


def on_disconnect(client: mqtt.Client):
    client.connected_flag = False
    client.disconnect_flag = True


def on_message(message: mqtt.MQTTMessage):
    message_content = str(message.payload.decode("utf-8"))
    logging.debug(f"message received (topic={message.topic}, content={message_content})", )

    maybe_sensor_data = sensor_data_from_json_string(message_content)

    if maybe_sensor_data is not None:
        SENSOR_DATA_BUFFER.append(maybe_sensor_data)


def init_mqtt_client(config: Config) -> mqtt.Client:
    logging.info(f"Connecting to MQTT broker '{config.mqtt.broker}' as client '{config.mqtt.client_id}-mysql' ...")

    client = mqtt.Client(client_id=f"{config.mqtt.client_id}-mysql", clean_session=False)
    client.connected_flag = False

    client.on_message = lambda _, __, message: on_message(message)
    client.on_connect = lambda _client, _, __, rc: on_connect(_client, config, rc)
    client.on_disconnect = lambda _client, _, __: on_disconnect(_client)

    client.username_pw_set(username=config.mqtt.username, password=config.mqtt.password)

    while True:
        # noinspection PyBroadException
        try:
            client.connect(config.mqtt.broker)
            break
        except:
            logging.error("Initial connection failed! Trying again ...")
            time.sleep(2)

    client.loop_start()

    while not client.connected_flag and not client.bad_connection_flag:
        logging.debug("Waiting for connection ...")
        time.sleep(1)

    if client.bad_connection_flag:
        client.loop_stop()
        logging.error("Initial connection failed unrecoverable!")
        sys.exit(1)

    logging.info(f"Done connecting to MQTT broker '{config.mqtt.broker}' as client '{config.mqtt.client_id}-mysql'.")

    return client


def init_database_manager(config: Config) -> DatabaseManager:
    logging.info(f"Connecting to database '{config.database.host}' ...")

    db_manager = DatabaseManager(
        db_host=config.database.host,
        db_user=config.database.username,
        db_password=config.database.password,
        db_schema=config.database.schema,
        db_table=config.database.table
    )
    db_manager.init_database()

    if db_manager.is_connected():
        logging.info(f"Done connecting to database '{config.database.host}'.")
    else:
        logging.error(f"Failed connecting to database '{config.database.host}'!")

    return db_manager


def main(mqtt_client: mqtt.Client, db_manager: DatabaseManager, config: DatabaseConfig):
    was_disconnected = False
    start_time = time.time()

    while True:
        if not mqtt_client.connected_flag:
            was_disconnected = True
            logging.error("MQTT broker not connected! Trying to reconnect...")  # reconnect is handled automatically
            time.sleep(2)
            continue

        if was_disconnected:
            was_disconnected = False
            logging.error("Successfully reconnected!")

        must_write = time.time() - start_time > config.write_frequency_seconds

        time.sleep(0.05)  # decrease CPU usage

        if must_write and len(SENSOR_DATA_BUFFER) > 0:
            logging.info(f"Saving sensor data buffer (of size {len(SENSOR_DATA_BUFFER)}) to database...")
            write_success = db_manager.write_sensor_data(SENSOR_DATA_BUFFER)

            if write_success:
                SENSOR_DATA_BUFFER.clear()
                start_time = time.time()
                logging.info(f"Done saving sensor data buffer to database.")
            else:
                logging.error(f"Failed saving sensor data buffer (of size {len(SENSOR_DATA_BUFFER)}) to database!")
                db_manager.try_reconnect()


if __name__ == '__main__':
    logging.basicConfig(format='%(asctime)s - %(levelname)s : %(message)s', level=logging.DEBUG)
    logging.info("Starting MySQL data collector")

    if len(sys.argv) >= 2:
        CONFIG_FILE_PATH = sys.argv[1]

    maybe_config = config_from_file(CONFIG_FILE_PATH)

    if maybe_config is None:
        exit(1)

    logging.basicConfig(format='%(asctime)s - %(levelname)s : %(message)s', level=maybe_config.database.log_level, force=True)

    _database_manager = init_database_manager(maybe_config)
    _mqtt_client = init_mqtt_client(maybe_config)

    try:
        main(_mqtt_client, _database_manager, maybe_config.database)
        logging.basicConfig(format='%(asctime)s - %(levelname)s : %(message)s', level=logging.DEBUG, force=True)
    except KeyboardInterrupt:
        logging.basicConfig(format='%(asctime)s - %(levelname)s : %(message)s', level=logging.DEBUG, force=True)
        logging.info("Data collection ended due to keyboard interrupt!")

    logging.info("Closing MQTT connection ...")
    _mqtt_client.loop_stop()
    time.sleep(5)
    _mqtt_client.disconnect()
    logging.info("Done closing MQTT connection.")

    logging.info("Closing database connection ...")
    if not _database_manager.close_connection():
        logging.error("Failed closing database connection! Will try to reconnect to save last data!")

        while not _database_manager.try_reconnect() or not _database_manager.close_connection():
            time.sleep(2)
            logging.info("Trying again ...")

    logging.info("Done closing database connection.")
