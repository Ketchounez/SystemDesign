const dbName = "hotel_booking_lab4";
const database = db.getSiblingDB(dbName);

database.createCollection("bookings");

database.bookings.createIndex({ id: 1 }, { unique: true });
database.bookings.createIndex({ user_id: 1, status: 1 });
database.bookings.createIndex({ hotel_id: 1, check_in: 1, check_out: 1 });

print("Mongo init complete: bookings collection and indexes created.");
