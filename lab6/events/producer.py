#!/usr/bin/env python3
"""Producer: публикует доменные события в RabbitMQ (topic exchange)."""

from __future__ import annotations

import argparse
import sys

import pika

from common import (
    EXCHANGE_NAME,
    EXCHANGE_TYPE,
    RABBITMQ_HOST,
    RABBITMQ_PASSWORD,
    RABBITMQ_PORT,
    RABBITMQ_USER,
    RABBITMQ_VHOST,
    DomainEvent,
    build_event,
)


def connect() -> pika.BlockingConnection:
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


def publish(channel: pika.channel.Channel, event: DomainEvent) -> None:
    channel.basic_publish(
        exchange=EXCHANGE_NAME,
        routing_key=event.event_type,
        body=event.to_json().encode("utf-8"),
        properties=pika.BasicProperties(
            content_type="application/json",
            delivery_mode=2,
            message_id=event.event_id,
            type=event.event_type,
        ),
        mandatory=True,
    )
    print(f"published event_type={event.event_type} event_id={event.event_id}")


def demo_events() -> list[DomainEvent]:
    return [
        build_event(
            "user.registered",
            {
                "user_id": 101,
                "login": "demo_user",
                "name": "Ivan",
                "surname": "Petrov",
                "email": "ivan@example.com",
            },
            producer="hotel-booking-api",
        ),
        build_event(
            "hotel.created",
            {
                "hotel_id": 501,
                "name": "Grand Plaza",
                "city": "Moscow",
                "address": "Tverskaya 1",
                "created_by_user_id": 101,
            },
            producer="hotel-booking-api",
        ),
        build_event(
            "booking.created",
            {
                "booking_id": 9001,
                "user_id": 101,
                "hotel_id": 501,
                "check_in": "2026-06-01",
                "check_out": "2026-06-05",
                "status": "active",
            },
            producer="hotel-booking-api",
        ),
        build_event(
            "booking.cancelled",
            {
                "booking_id": 9001,
                "user_id": 101,
                "hotel_id": 501,
                "status": "cancelled",
            },
            producer="hotel-booking-api",
        ),
    ]


def main() -> int:
    parser = argparse.ArgumentParser(description="Publish domain events to RabbitMQ")
    parser.add_argument(
        "--demo",
        action="store_true",
        help="Publish demo event sequence for lab verification",
    )
    parser.add_argument("--event-type", help="Routing key / event type")
    parser.add_argument("--payload-json", help='JSON payload, e.g. {"booking_id":1}')
    args = parser.parse_args()

    events: list[DomainEvent]
    if args.demo:
        events = demo_events()
    elif args.event_type and args.payload_json:
        import json

        payload = json.loads(args.payload_json)
        events = [build_event(args.event_type, payload, producer="hotel-booking-api")]
    else:
        parser.print_help()
        return 1

    connection = connect()
    try:
        channel = connection.channel()
        channel.confirm_delivery()
        declare_topology(channel)
        for event in events:
            publish(channel, event)
    except pika.exceptions.UnroutableError as exc:
        print(f"failed to publish: {exc}", file=sys.stderr)
        return 2
    finally:
        connection.close()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
