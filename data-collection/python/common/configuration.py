import json
import logging
from dataclasses import dataclass
from typing import Optional


@dataclass
class MqttConfig:
    client_id: str
    broker: str
    username: str
    password: str
    topic: str


@dataclass
class DatabaseConfig:
    host: str
    username: str
    password: str
    schema: str
    table: str
    write_frequency_seconds: int
    log_level: str


@dataclass
class CsvConfig:
    output_file: str
    log_level: str


@dataclass
class Config:
    mqtt: MqttConfig
    database: DatabaseConfig
    csv: CsvConfig


def config_from_file(file_path: str) -> Optional[Config]:
    # noinspection PyBroadException
    try:
        with open(file_path, mode="r") as file:
            config_string = file.read()
            config_dict: dict = json.loads(config_string)

            return Config(
                mqtt=MqttConfig(
                    client_id=config_dict["MQTT"]["CLIENT_ID"],
                    broker=config_dict["MQTT"]["BROKER"],
                    username=config_dict["MQTT"]["USERNAME"],
                    password=config_dict["MQTT"]["PASSWORD"],
                    topic=config_dict["MQTT"]["TOPIC"],
                ),
                database=DatabaseConfig(
                    host=config_dict["DATABASE"]["HOST"],
                    username=config_dict["DATABASE"]["USERNAME"],
                    password=config_dict["DATABASE"]["PASSWORD"],
                    schema=config_dict["DATABASE"]["SCHEMA"],
                    table=config_dict["DATABASE"]["TABLE"],
                    write_frequency_seconds=config_dict["DATABASE"]["WRITE_FREQUENCY_SECONDS"],
                    log_level=config_dict["DATABASE"]["LOG_LEVEL"]
                ),
                csv=CsvConfig(
                    output_file=config_dict["CSV"]["OUTPUT_FILE"],
                    log_level=config_dict["CSV"]["LOG_LEVEL"]
                )
            )
    except FileNotFoundError:
        logging.error(f"Config file '{file_path}' not found!")
    except KeyError:
        logging.error("Config in wrong format!")
    except Exception as ex:
        logging.exception("Could not parse config file!", ex)

    return None
