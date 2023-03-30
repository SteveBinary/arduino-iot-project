# Data collection for the Arduino IoT project

The data from the Arduino IoT sensor is sent to an MQTT broker. \
Measurements of all sensors happens at the same time, so there will be new data published at the same time to each topic. \
There are three methods used for collecting and storing the data, all of them subscribe to the MQTT broker:

* [Telegraf and InfluxDB](#telegraf-and-influxdb)
* [Python script and MySQL database](#python-script-and-mysql-database)
* [Python script and CSV file](#python-script-and-csv-file)

## Topics and data

| Topic                       | Data                         | Unit | Example                                                                                                                                  |
|-----------------------------|------------------------------|------|------------------------------------------------------------------------------------------------------------------------------------------|
| `dhai/Ulm/steve/all`        | all readings of the below    |      | `{"tempi": 23.56, "pressure": 961.94, "humidity": 47.91, "airquality": 94.43, "light": 1.08, "sequence": 1312, "timestamp": 1679270633}` |
| `dhai/Ulm/steve/tempi`      | temperature inside           | °C   | `{"value": 23.50, "unit": "°C", "sequence": 1389, "timestamp": 1679270816}`                                                              |
| `dhai/Ulm/steve/pressure`   | air pressure                 | hPa  | `{"value": 961.91, "unit": "hPa", "sequence": 1392, "timestamp": 1679270823}`                                                            |
| `dhai/Ulm/steve/humidity`   | humidity                     | %    | `{"value": 47.89, "unit": "%", "sequence": 1393, "timestamp": 1679270826}`                                                               |
| `dhai/Ulm/steve/airquality` | air quality (gas resistance) | KOhm | `{"value": 94.96, "unit": "KOhm", "sequence": 1395, "timestamp": 1679270831}`                                                            |
| `dhai/Ulm/steve/light`      | light intensity              | %    | `{"value": 1.47, "unit": "%", "sequence": 1398, "timestamp": 1679270838}`                                                                |

## Telegraf and InfluxDB

## Python script and MySQL database

This method uses the `dhai/Ulm/steve/all` topic.
That makes synchronization of the topics unnecessary.
Synchronization would otherwise be mandatory because the database schema expects all sensor data in a single entry.

Docker command for starting a MySQL database for testing:

```shell
docker run --name MySQL-test-db -p 3306:3306 -v mysql-test-db:/var/lib/mysql -e MYSQL_ROOT_PASSWORD=super-secret -e MYSQL_USER=sensor -e MYSQL_PASSWORD=sensor-secret -e MYSQL_DATABASE=test_db -e TZ="Europe/Berlin" -d mysql:8.0.32
```

### SQL schema

The topic does not need to be stored. \
The columns (tempi, ...) are sensor values and the name corresponds to the topic.

```sql
CREATE TABLE IF NOT EXISTS test_db.sensor_data (
    id         INT unsigned NOT NULL AUTO_INCREMENT,
    timestamp  TIMESTAMP    NOT NULL,
    sequence   INT unsigned NOT NULL,
    tempi      FLOAT        NOT NULL,
    pressure   FLOAT        NOT NULL,
    humidity   FLOAT        NOT NULL,
    airquality FLOAT        NOT NULL,
    light      FLOAT        NOT NULL,
    UNIQUE  KEY timestamp_index (timestamp) USING BTREE,  -- create an index on timestamp 
    PRIMARY KEY (id)
);
```

## Python script and CSV file
