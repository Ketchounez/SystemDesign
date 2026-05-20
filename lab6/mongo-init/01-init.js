const dbName = "hotel_booking_lab6";
const database = db.getSiblingDB(dbName);

database.createCollection("bookings");
database.createCollection("users_read_model");
database.createCollection("booking_read_model");
database.createCollection("hotel_catalog_view");

database.bookings.createIndex({ id: 1 }, { unique: true });
database.bookings.createIndex({ user_id: 1, status: 1 });
database.bookings.createIndex({ hotel_id: 1, check_in: 1, check_out: 1 });

database.booking_read_model.createIndex({ user_id: 1, booking_id: 1 }, { unique: true });
database.booking_read_model.createIndex({ user_id: 1, status: 1 });

database.hotel_catalog_view.createIndex({ hotel_id: 1 }, { unique: true });
database.hotel_catalog_view.createIndex({ city: 1 });

print("Mongo init complete: write and CQRS read-model collections created.");
