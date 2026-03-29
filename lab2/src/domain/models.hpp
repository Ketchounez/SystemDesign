#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

namespace lab2 {

enum class ServiceErrorKind {
  kBadRequest,
  kUnauthorized,
  kForbidden,
  kNotFound,
  kConflict,
};

class ServiceError final : public std::runtime_error {
 public:
  ServiceError(ServiceErrorKind kind, std::string code, std::string message)
      : std::runtime_error(std::move(message)),
        kind_(kind),
        code_(std::move(code)) {}

  ServiceErrorKind GetKind() const noexcept { return kind_; }
  const std::string& GetCode() const noexcept { return code_; }

 private:
  ServiceErrorKind kind_;
  std::string code_;
};

struct User {
  std::int64_t id{0};
  std::string login;
  std::string name;
  std::string surname;
  std::string email;
  std::string password_hash;
};

struct Hotel {
  std::int64_t id{0};
  std::string name;
  std::string city;
  std::string address;
  std::string description;
  std::int64_t created_by_user_id{0};
};

enum class BookingStatus {
  kActive,
  kCancelled,
};

inline std::string ToString(BookingStatus status) {
  return status == BookingStatus::kActive ? "active" : "cancelled";
}

struct Booking {
  std::int64_t id{0};
  std::int64_t user_id{0};
  std::int64_t hotel_id{0};
  std::string check_in;
  std::string check_out;
  BookingStatus status{BookingStatus::kActive};
};

struct Session {
  std::string token;
  std::int64_t user_id{0};
  std::string login;
};

struct AuthContext {
  std::int64_t user_id{0};
  std::string login;
};

struct RegisterUserRequest {
  std::string login;
  std::string name;
  std::string surname;
  std::string email;
  std::string password;
};

struct LoginRequest {
  std::string login;
  std::string password;
};

struct CreateHotelRequest {
  std::string name;
  std::string city;
  std::string address;
  std::string description;
};

struct CreateBookingRequest {
  std::int64_t hotel_id{0};
  std::string check_in;
  std::string check_out;
};

}
