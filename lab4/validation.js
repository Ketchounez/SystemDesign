// Создание валидации коллекции bookings через $jsonSchema
// Запуск: mongosh "mongodb://localhost:27017/hotel_booking_lab4" validation.js

const dbName = "hotel_booking_lab4";
const database = db.getSiblingDB(dbName);

database.runCommand({
  collMod: "bookings",
  validator: {
    $jsonSchema: {
      bsonType: "object",
      required: [
        "id",
        "user_id",
        "hotel_id",
        "check_in",
        "check_out",
        "status",
        "created_at"
      ],
      properties: {
        id: { bsonType: ["int", "long"], minimum: 1 },
        user_id: { bsonType: ["int", "long"], minimum: 1 },
        hotel_id: { bsonType: ["int", "long"], minimum: 1 },
        check_in: { bsonType: "string", pattern: "^\\d{4}-\\d{2}-\\d{2}$" },
        check_out: { bsonType: "string", pattern: "^\\d{4}-\\d{2}-\\d{2}$" },
        guests_count: { bsonType: ["int", "long"], minimum: 1, maximum: 10 },
        status: { enum: ["active", "cancelled"] },
        total_price: { bsonType: ["int", "long", "double", "decimal"], minimum: 0 },
        created_at: { bsonType: "date" },
        cancelled_at: { bsonType: ["date", "null"] },
        user_snapshot: {
          bsonType: "object",
          required: ["login", "full_name"],
          properties: {
            login: { bsonType: "string", minLength: 3, maxLength: 40 },
            full_name: { bsonType: "string", minLength: 3, maxLength: 120 }
          }
        },
        hotel_snapshot: {
          bsonType: "object",
          required: ["name", "city", "price_per_night"],
          properties: {
            name: { bsonType: "string", minLength: 2, maxLength: 120 },
            city: { bsonType: "string", minLength: 2, maxLength: 80 },
            price_per_night: { bsonType: ["int", "long", "double", "decimal"], minimum: 0 }
          }
        }
      }
    }
  },
  validationLevel: "strict",
  validationAction: "error"
});

print("Validation applied for bookings collection.");

print("\nTrying to insert invalid booking...");
const invalid = database.bookings.insertOne(
  {
    id: 999,
    user_id: 1,
    hotel_id: 1,
    check_in: "2026-06-01",
    check_out: "2026-06-02",
    status: "pending", // invalid enum value
    created_at: new Date()
  },
  { bypassDocumentValidation: false }
);
printjson(invalid);
