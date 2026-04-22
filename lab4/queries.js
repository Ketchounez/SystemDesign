// MongoDB CRUD-запросы для гибридной модели варианта 13
// Запуск: mongosh "mongodb://localhost:27017/hotel_booking_lab4" queries.js

const dbName = "hotel_booking_lab4";
const database = db.getSiblingDB(dbName);

print("\n1) CREATE: создание бронирования");
const createBookingResult = database.bookings.insertOne({
  id: 1001,
  user_id: 2,
  hotel_id: 3,
  check_in: "2026-05-10",
  check_out: "2026-05-13",
  guests_count: 2,
  status: "active",
  total_price: 27000,
  created_at: new Date(),
  cancelled_at: null,
  user_snapshot: {
    login: "user02",
    full_name: "Petr Ivanov"
  },
  hotel_snapshot: {
    name: "Nevsky Place",
    city: "Saint Petersburg",
    price_per_night: 9000
  }
});
printjson(createBookingResult);

print("\n2) READ: поиск бронирований пользователя ($eq, $ne)");
printjson(
  database.bookings.find({
    user_id: { $eq: 2 },
    status: { $ne: "cancelled" }
  }).toArray()
);

print("\n3) READ: поиск бронирований по условиям ($and, $gt, $lt, $in, $or)");
printjson(
  database.bookings.find({
    $and: [
      { total_price: { $gt: 3000 } },
      { total_price: { $lt: 50000 } },
      { "hotel_snapshot.city": { $in: ["Moscow", "Kazan", "Saint Petersburg"] } },
      { $or: [{ guests_count: 1 }, { guests_count: 2 }] }
    ]
  }).limit(3).toArray()
);

print("\n4) READ: получить бронирование по id");
printjson(database.bookings.findOne({ id: 1001 }));

print("\n5) UPDATE: отмена бронирования ($set)");
printjson(
  database.bookings.updateOne(
    { id: 1001, status: { $ne: "cancelled" } },
    { $set: { status: "cancelled", cancelled_at: new Date() } }
  )
);

print("\n6) UPDATE массивов в snapshot-примере ($addToSet, $pull)");
database.bookings.updateOne(
  { id: 1001 },
  { $setOnInsert: { tags: [] } },
  { upsert: false }
);
printjson(
  database.bookings.updateOne({ id: 1001 }, { $addToSet: { tags: "priority" } })
);
printjson(
  database.bookings.updateOne({ id: 1001 }, { $pull: { tags: "obsolete" } })
);

print("\n7) DELETE: удаление тестового бронирования");
printjson(database.bookings.deleteOne({ id: 1001 }));

print("\n8) Aggregation pipeline (опционально): выручка по городам");
printjson(
  database.bookings.aggregate([
    { $match: { status: "active", total_price: { $gt: 0 } } },
    {
      $group: {
        _id: "$hotel_snapshot.city",
        bookings_count: { $sum: 1 },
        revenue: { $sum: "$total_price" }
      }
    },
    { $project: { _id: 0, city: "$_id", bookings_count: 1, revenue: 1 } },
    { $sort: { revenue: -1 } }
  ]).toArray()
);
