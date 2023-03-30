import json
import logging
import time
from dataclasses import dataclass
from datetime import datetime
from typing import Optional


@dataclass
class SensorData:
    tempi: float
    pressure: float
    humidity: float
    airquality: float
    light: float
    sequence: int
    timestamp: datetime

    def to_csv_string(self) -> str:
        return f"{round(time.mktime(self.timestamp.timetuple()))},{self.sequence},{self.tempi},{self.pressure},{self.humidity},{self.airquality},{self.light}\n"

    @staticmethod
    def csv_header() -> str:
        return "timestamp,sequence,tempi,pressure,humidity,airquality,light\n"


def sensor_data_from_json_string(json_string: str) -> Optional[SensorData]:
    # noinspection PyBroadException
    try:
        sensor_data_dict = json.loads(json_string)

        return SensorData(
            tempi=sensor_data_dict["tempi"],
            pressure=sensor_data_dict["pressure"],
            humidity=sensor_data_dict["humidity"],
            airquality=sensor_data_dict["airquality"],
            light=sensor_data_dict["light"],
            sequence=sensor_data_dict["sequence"],
            timestamp=datetime.fromtimestamp(sensor_data_dict["timestamp"]),
        )
    except Exception as ex:
        logging.exception(f"Could not construct SensorData object from: {json_string}", ex)

    return None
