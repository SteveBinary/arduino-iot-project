import logging
from typing import Union

import _mysql_connector
import mysql
from mysql.connector import connect as connect_to_mysql_db, MySQLConnection, CMySQLConnection
from mysql.connector.pooling import PooledMySQLConnection

from python.common.sensor_data import SensorData


def sql_statement_create_table_if_not_exists(schema: str, table: str) -> str:
    return f"""
        CREATE TABLE IF NOT EXISTS {schema}.{table} (
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
        """


def sql_statement_insert_into_table(schema: str, table: str) -> str:
    return f"""
        INSERT INTO {schema}.{table}
        (timestamp, sequence, tempi, pressure, humidity, airquality, light)
        VALUES (%s, %s, %s, %s, %s, %s, %s);
        """


class DatabaseManager:
    db: Union[PooledMySQLConnection, MySQLConnection, CMySQLConnection, None]
    db_initialized: bool
    db_host: str
    db_user: str
    db_password: str
    db_schema: str
    db_table: str

    def __init__(self, db_host: str, db_user: str, db_password: str, db_schema: str, db_table: str):
        self.db_host = db_host
        self.db_user = db_user
        self.db_password = db_password
        self.db_schema = db_schema
        self.db_table = db_table
        self.db_initialized = False

        try:
            self.db = connect_to_mysql_db(host=self.db_host, user=self.db_user, passwd=self.db_password)
        except mysql.connector.errors.DatabaseError:
            self.db = None

    def init_database(self):
        if self.db is None:
            return
        # noinspection PyBroadException
        try:
            with self.db.cursor() as cursor:
                cursor.execute(sql_statement_create_table_if_not_exists(self.db_schema, self.db_table))
                self.db.commit()
                self.db_initialized = True
        except:
            self.db_initialized = False

    def is_connected(self) -> bool:
        return self.db is not None and self.db.is_connected()

    def try_reconnect(self):
        if self.db is not None and not self.db.is_connected():
            try:
                self.db.reconnect()
            except mysql.connector.errors.InterfaceError:
                logging.info("Reconnecting to database failed!")
        else:
            try:
                self.db = connect_to_mysql_db(host=self.db_host, user=self.db_user, passwd=self.db_password)
            except mysql.connector.errors.DatabaseError:
                pass

    def close_connection(self) -> bool:
        if self.db is not None:
            try:
                self.db.commit()
                self.db.close()
                return True
            except _mysql_connector.MySQLInterfaceError:
                return False

    def write_sensor_data(self, sensor_data: list[SensorData]) -> bool:
        if len(sensor_data) == 0:
            return True

        if self.db is None:
            return False

        if not self.db_initialized:
            self.init_database()

        prepared_sensor_data = [
            (
                data.timestamp,
                data.sequence,
                data.tempi,
                data.pressure,
                data.humidity,
                data.airquality,
                data.light,
            )
            for data in sensor_data
        ]

        try:
            with self.db.cursor() as cursor:
                cursor.executemany(sql_statement_insert_into_table(self.db_schema, self.db_table), prepared_sensor_data)
                self.db.commit()
                return True
        except mysql.connector.errors.OperationalError:
            return False
