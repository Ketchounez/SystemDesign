#!/usr/bin/env python3
"""Consumer: обновляет CQRS read-model в MongoDB по событиям из RabbitMQ."""

from __future__ import annotations

import sys
import time

import pika
from pymongo import MongoClient

from common import (
    EXCHANGE_NAME,
    EXCHANGE_TYPE,
    MONGO_DB,
    MONGO_URI,
    RABBITMQ_HOST,
    RABBITMQ_PASSWORD,
    RABBITMQ_PORT,
    RABBITMQ_USER,
    RABBITMQ_VHOST,
    READ_MODEL_QUEUE,
    READ_MODEL_ROUTING_KEYS,
    DomainEvent,
)


def connect_rabbit() -> pika.BlockingConnection:
    credentials = pika.PlainCredentials(RABBITMQ_USER, RABBITMQ_PASSWORD)
    params = pika.ConnectionParameters(
        host=RABBITMQ_HOST,
        port=RABBITMQ_PORT,
        virtual_host=RABBITMQ_VHOST,
        credentials=credentials,
    )
    return pika.BlockingConnection(params)


def declare_topology(channel: pika.channel.Channel) -> None:
    channel.exchange_declare(
        exchange=EXCHANGE_NAME,
        exchange_type=EXCHANGE_TYPE,
        durable=True,
    )
    channel.queue_declare(queue=READ_MODEL_QUEUE, durable=True)
    for routing_key in READ_MODEL_ROUTING_KEYS:
        channel.queue_bind(
            queue=READ_MODEL_QUEUE,
            exchange=EXCHANGE_NAME,
            routing_key=routing_key,
        )


def apply_event(mongo_db, event: DomainEvent) -> None:
    if event.event_type == "user.registered":
        mongo_db["users_read_model"].update_one(
            {"user_id": event.payload["user_id"]},
            {
                "$set": {
                    "user_id": event.payload["user_id"],
                    "login": event.payload["login"],
                    "name": event.payload["name"],
                    "surname": event.payload["surname"],
                    "email": event.payload["email"],
                    "updated_at": event.occurred_at,
                }
            },
            upsert=True,
        )
        return

    if event.event_type == "hotel.created":
        mongo_db["hotel_catalog_view"].update_one(
            {"hotel_id": event.payload["hotel_id"]},
            {
                "$set": {
                    "hotel_id": event.payload["hotel_id"],
                    "name": event.payload["name"],
                    "city": event.payload["city"],
                    "address": event.payload["address"],
                    "created_by_user_id": event.payload["created_by_user_id"],
                    "updated_at": event.occurred_at,
                }
            },
            upsert=True,
        )
        return

    if event.event_type == "booking.created":
        mongo_db["booking_read_model"].update_one(
            {
                "user_id": event.payload["user_id"],
                "booking_id": event.payload["booking_id"],
            },
            {
                "$set": {
                    "booking_id": event.payload["booking_id"],
                    "user_id": event.payload["user_id"],
                    "hotel_id": event.payload["hotel_id"],
                    "check_in": event.payload["check_in"],
                    "check_out": event.payload["check_out"],
                    "status": event.payload["status"],
                    "updated_at": event.occurred_at,
                }
            },
            upsert=True,
        )
        return

    if event.event_type == "booking.cancelled":
        mongo_db["booking_read_model"].update_one(
            {
                "user_id": event.payload["user_id"],
                "booking_id": event.payload["booking_id"],
            },
            {
                "$set": {
                    "status": event.payload["status"],
                    "updated_at": event.occurred_at,
                }
            },
            upsert=True,
        )
        return

    print(f"skip unsupported event_type={event.event_type}", file=sys.stderr)


def wait_for_mongo(client: MongoClient, timeout_seconds: int = 60) -> None:
    deadline = time.time() + timeout_seconds
    while time.time() < deadline:
        try:
            client.admin.command("ping")
            return
        except Exception:
            time.sleep(2)
    raise RuntimeError("MongoDB is not available")


def main() -> int:
    mongo_client = MongoClient(MONGO_URI)
    wait_for_mongo(mongo_client)
    mongo_db = mongo_client[MONGO_DB]

    connection = connect_rabbit()
    channel = connection.channel()
    channel.basic_qos(prefetch_count=10)
    declare_topology(channel)

    def on_message(ch, method, properties, body):  # noqa: ARG001
        try:
            event = DomainEvent.from_json(body.decode("utf-8"))
            apply_event(mongo_db, event)
            print(
                f"processed event_type={event.event_type} "
                f"event_id={event.event_id} delivery_tag={method.delivery_tag}"
            )
            ch.basic_ack(delivery_tag=method.delivery_tag)
        except Exception as exc:  # noqa: BLE001
            print(f"failed to process message: {exc}", file=sys.stderr)
            ch.basic_nack(delivery_tag=method.delivery_tag, requeue=True)

    channel.basic_consume(queue=READ_MODEL_QUEUE, on_message_callback=on_message)
    print(f"consumer started queue={READ_MODEL_QUEUE}")
    channel.start_consuming()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
