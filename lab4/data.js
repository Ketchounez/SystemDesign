// Запуск:
// mongosh "mongodb://localhost:27017/hotel_booking_lab4" data.js

const dbName = "hotel_booking_lab4";
const database = db.getSiblingDB(dbName);

database.bookings.drop();
database.createCollection("bookings");

const bookings = [];
for (let i = 1; i <= 10; i += 1) {
  const checkInDay = i.toString().padStart(2, "0");
  const checkOutDay = (i + 1).toString().padStart(2, "0");
  const status = i % 4 === 0 ? "cancelled" : "active";

  bookings.push({
    id: i,
    user_id: i,
    hotel_id: i,
    check_in: `2026-03-${checkInDay}`,
    check_out: `2026-03-${checkOutDay}`,
    guests_count: (i % 3) + 1,
    status,
    total_price: 3000 + i * 500,
    created_at: new Date(`2026-02-${checkInDay}T09:00:00Z`),
    cancelled_at: status === "cancelled" ? new Date(`2026-02-${checkOutDay}T09:00:00Z`) : null,
    user_snapshot: {
      login: `user${i.toString().padStart(2, "0")}`,
      full_name: `User ${i}`
    },
    hotel_snapshot: {
      name: `Hotel ${i}`,
      city: i % 2 === 0 ? "Moscow" : "Kazan",
      price_per_night: 3000 + i * 500
    }
  });
}

database.bookings.insertMany(bookings);
database.bookings.createIndex({ id: 1 }, { unique: true });
database.bookings.createIndex({ user_id: 1, status: 1 });
database.bookings.createIndex({ hotel_id: 1, check_in: 1, check_out: 1 });

print("Seed completed");
print(`bookings: ${database.bookings.countDocuments()}`);
