"""Общие константы и формат событий для RabbitMQ."""

from __future__ import annotations

import json
import os
import uuid
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from typing import Any, Dict

EXCHANGE_NAME = "hotel.booking.events"
EXCHANGE_TYPE = "topic"

READ_MODEL_QUEUE = "hotel.booking.read_model.worker"
READ_MODEL_ROUTING_KEYS = ("user.registered", "hotel.created", "booking.created", "booking.cancelled")

RABBITMQ_HOST = os.getenv("RABBITMQ_HOST", "localhost")
RABBITMQ_PORT = int(os.getenv("RABBITMQ_PORT", "5672"))
RABBITMQ_USER = os.getenv("RABBITMQ_USER", "hotel")
RABBITMQ_PASSWORD = os.getenv("RABBITMQ_PASSWORD", "hotel")
RABBITMQ_VHOST = os.getenv("RABBITMQ_VHOST", "/")

MONGO_URI = os.getenv("MONGO_URI", "mongodb://mongo:27017/hotel_booking_lab6")
MONGO_DB = os.getenv("MONGO_DB", "hotel_booking_lab6")


def utc_now_iso() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat()


@dataclass
class DomainEvent:
    event_id: str
    event_type: str
    occurred_at: str
    producer: str
    payload: Dict[str, Any]

    def to_json(self) -> str:
        return json.dumps(asdict(self), ensure_ascii=False)

    @classmethod
    def from_json(cls, raw: str) -> "DomainEvent":
        data = json.loads(raw)
        return cls(
            event_id=data["event_id"],
            event_type=data["event_type"],
            occurred_at=data["occurred_at"],
            producer=data["producer"],
            payload=data["payload"],
        )


def build_event(event_type: str, payload: Dict[str, Any], producer: str) -> DomainEvent:
    return DomainEvent(
        event_id=str(uuid.uuid4()),
        event_type=event_type,
        occurred_at=utc_now_iso(),
        producer=producer,
        payload=payload,
    )
