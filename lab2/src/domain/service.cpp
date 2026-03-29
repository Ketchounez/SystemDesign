#include "domain/service.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <optional>
#include <regex>
#include <sstream>

namespace lab2 {

namespace {

std::string ToLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

std::string Fnv1aHex(std::string_view value) {
  constexpr std::uint64_t kOffsetBasis = 14695981039346656037ULL;
  constexpr std::uint64_t kPrime = 1099511628211ULL;

  std::uint64_t hash = kOffsetBasis;
  for (unsigned char ch : value) {
    hash ^= ch;
    hash *= kPrime;
  }

  std::ostringstream out;
  out << std::hex << std::setfill('0') << std::setw(16) << hash;
  return out.str();
}

}

std::string HotelBookingService::Trim(const std::string& value) {
  const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
    return std::isspace(ch) != 0;
  });
  const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
    return std::isspace(ch) != 0;
  }).base();

  if (begin >= end) {
    return {};
  }

  return std::string(begin, end);
}

std::string HotelBookingService::Normalize(const std::string& value) {
  return ToLower(Trim(value));
}

bool HotelBookingService::IsBlank(const std::string& value) {
  return Trim(value).empty();
}

bool HotelBookingService::ContainsCaseInsensitive(std::string_view haystack, std::string_view needle) {
  return ToLower(std::string(haystack)).find(ToLower(std::string(needle))) != std::string::npos;
}

void HotelBookingService::ValidateDate(const std::string& value, const std::string& field_name) {
  static const std::regex kDatePattern(R"(^\d{4}-\d{2}-\d{2}$)");
  if (!std::regex_match(value, kDatePattern)) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "invalid_date",
        "Field '" + field_name + "' must be in YYYY-MM-DD format");
  }
}

bool HotelBookingService::DatesOverlap(const Booking& left, const Booking& right) {
  return !(left.check_out <= right.check_in || left.check_in >= right.check_out);
}

std::string HotelBookingService::HashPassword(const std::string& value) {
  return Fnv1aHex("password:" + value);
}

std::string HotelBookingService::GenerateToken(
    std::string_view login,
    std::int64_t user_id,
    std::int64_t session_seq) {
  const auto seed =
      std::string(login) + ":" + std::to_string(user_id) + ":" + std::to_string(session_seq);
  return "session-" + Fnv1aHex(seed);
}

const User& HotelBookingService::RequireUserUnlocked(std::int64_t user_id) const {
  const auto it = users_.find(user_id);
  if (it == users_.end()) {
    throw ServiceError(ServiceErrorKind::kNotFound, "user_not_found", "User was not found");
  }
  return it->second;
}

const Hotel& HotelBookingService::RequireHotelUnlocked(std::int64_t hotel_id) const {
  const auto it = hotels_.find(hotel_id);
  if (it == hotels_.end()) {
    throw ServiceError(ServiceErrorKind::kNotFound, "hotel_not_found", "Hotel was not found");
  }
  return it->second;
}

const Booking& HotelBookingService::RequireBookingUnlocked(std::int64_t booking_id) const {
  const auto it = bookings_.find(booking_id);
  if (it == bookings_.end()) {
    throw ServiceError(ServiceErrorKind::kNotFound, "booking_not_found", "Booking was not found");
  }
  return it->second;
}

User HotelBookingService::RegisterUser(const RegisterUserRequest& request) {
  auto login = Normalize(request.login);
  auto name = Trim(request.name);
  auto surname = Trim(request.surname);
  auto email = Trim(request.email);
  const auto password = request.password;

  if (login.empty() || name.empty() || surname.empty() || email.empty() || password.empty()) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "validation_error",
        "login, name, surname, email and password are required");
  }

  std::lock_guard<std::mutex> lock(mutex_);
  if (user_ids_by_login_.count(login) != 0) {
    throw ServiceError(
        ServiceErrorKind::kConflict,
        "login_already_exists",
        "User with this login already exists");
  }

  User user;
  user.id = next_user_id_++;
  user.login = login;
  user.name = name;
  user.surname = surname;
  user.email = email;
  user.password_hash = HashPassword(password);

  users_.emplace(user.id, user);
  user_ids_by_login_[user.login] = user.id;
  return user;
}

Session HotelBookingService::Login(const LoginRequest& request) {
  const auto login = Normalize(request.login);
  if (login.empty() || request.password.empty()) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "validation_error",
        "login and password are required");
  }

  std::lock_guard<std::mutex> lock(mutex_);
  const auto id_it = user_ids_by_login_.find(login);
  if (id_it == user_ids_by_login_.end()) {
    throw ServiceError(
        ServiceErrorKind::kUnauthorized,
        "invalid_credentials",
        "Invalid login or password");
  }

  const auto& user = users_.at(id_it->second);
  if (user.password_hash != HashPassword(request.password)) {
    throw ServiceError(
        ServiceErrorKind::kUnauthorized,
        "invalid_credentials",
        "Invalid login or password");
  }

  Session session;
  session.user_id = user.id;
  session.login = user.login;
  session.token = GenerateToken(user.login, user.id, next_session_id_++);
  sessions_[session.token] = session;
  return session;
}

std::optional<Session> HotelBookingService::Authenticate(const std::string& token) const {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto it = sessions_.find(token);
  if (it == sessions_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<User> HotelBookingService::GetUserById(std::int64_t user_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto it = users_.find(user_id);
  if (it == users_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<User> HotelBookingService::FindUserByLogin(const std::string& login) const {
  const auto normalized_login = Normalize(login);
  std::lock_guard<std::mutex> lock(mutex_);
  const auto id_it = user_ids_by_login_.find(normalized_login);
  if (id_it == user_ids_by_login_.end()) {
    return std::nullopt;
  }
  return users_.at(id_it->second);
}

std::vector<User> HotelBookingService::SearchUsers(
    const std::string& name_mask,
    const std::string& surname_mask) const {
  const auto normalized_name = Trim(name_mask);
  const auto normalized_surname = Trim(surname_mask);
  if (normalized_name.empty() && normalized_surname.empty()) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "validation_error",
        "At least one search parameter must be provided");
  }

  std::vector<User> result;
  std::lock_guard<std::mutex> lock(mutex_);
  for (const auto& [_, user] : users_) {
    const bool name_matches = normalized_name.empty() ||
                              ContainsCaseInsensitive(user.name, normalized_name);
    const bool surname_matches = normalized_surname.empty() ||
                                 ContainsCaseInsensitive(user.surname, normalized_surname);
    if (name_matches && surname_matches) {
      result.push_back(user);
    }
  }

  std::sort(result.begin(), result.end(), [](const User& left, const User& right) {
    return left.id < right.id;
  });
  return result;
}

Hotel HotelBookingService::CreateHotel(
    const CreateHotelRequest& request,
    std::int64_t created_by_user_id) {
  auto name = Trim(request.name);
  auto city = Trim(request.city);
  auto address = Trim(request.address);
  auto description = Trim(request.description);

  if (name.empty() || city.empty() || address.empty()) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "validation_error",
        "name, city and address are required");
  }

  std::lock_guard<std::mutex> lock(mutex_);
  RequireUserUnlocked(created_by_user_id);

  Hotel hotel;
  hotel.id = next_hotel_id_++;
  hotel.name = name;
  hotel.city = city;
  hotel.address = address;
  hotel.description = description;
  hotel.created_by_user_id = created_by_user_id;

  hotels_.emplace(hotel.id, hotel);
  return hotel;
}

std::optional<Hotel> HotelBookingService::GetHotelById(std::int64_t hotel_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto it = hotels_.find(hotel_id);
  if (it == hotels_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::vector<Hotel> HotelBookingService::ListHotels(const std::optional<std::string>& city) const {
  const auto normalized_city = city.has_value() ? Normalize(*city) : std::string{};
  std::vector<Hotel> result;
  std::lock_guard<std::mutex> lock(mutex_);
  for (const auto& [_, hotel] : hotels_) {
    if (!normalized_city.empty() && Normalize(hotel.city) != normalized_city) {
      continue;
    }
    result.push_back(hotel);
  }

  std::sort(result.begin(), result.end(), [](const Hotel& left, const Hotel& right) {
    return left.id < right.id;
  });
  return result;
}

Booking HotelBookingService::CreateBooking(const CreateBookingRequest& request, std::int64_t user_id) {
  ValidateDate(request.check_in, "check_in");
  ValidateDate(request.check_out, "check_out");
  if (request.check_in >= request.check_out) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "invalid_date_range",
        "check_in must be earlier than check_out");
  }

  std::lock_guard<std::mutex> lock(mutex_);
  RequireUserUnlocked(user_id);
  RequireHotelUnlocked(request.hotel_id);

  Booking candidate;
  candidate.user_id = user_id;
  candidate.hotel_id = request.hotel_id;
  candidate.check_in = request.check_in;
  candidate.check_out = request.check_out;
  candidate.status = BookingStatus::kActive;

  for (const auto& [_, booking] : bookings_) {
    if (booking.hotel_id != candidate.hotel_id || booking.status != BookingStatus::kActive) {
      continue;
    }
    if (DatesOverlap(candidate, booking)) {
      throw ServiceError(
          ServiceErrorKind::kConflict,
          "booking_dates_conflict",
          "Hotel is already booked for the selected dates");
    }
  }

  candidate.id = next_booking_id_++;
  bookings_.emplace(candidate.id, candidate);
  return candidate;
}

std::vector<Booking> HotelBookingService::ListUserBookings(
    std::int64_t requester_user_id,
    std::int64_t target_user_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  RequireUserUnlocked(requester_user_id);
  RequireUserUnlocked(target_user_id);

  if (requester_user_id != target_user_id) {
    throw ServiceError(
        ServiceErrorKind::kForbidden,
        "forbidden",
        "You can only access your own bookings");
  }

  std::vector<Booking> result;
  for (const auto& [_, booking] : bookings_) {
    if (booking.user_id == target_user_id) {
      result.push_back(booking);
    }
  }

  std::sort(result.begin(), result.end(), [](const Booking& left, const Booking& right) {
    return left.id < right.id;
  });
  return result;
}

void HotelBookingService::CancelBooking(std::int64_t requester_user_id, std::int64_t booking_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  RequireUserUnlocked(requester_user_id);

  const auto it = bookings_.find(booking_id);
  if (it == bookings_.end()) {
    throw ServiceError(ServiceErrorKind::kNotFound, "booking_not_found", "Booking was not found");
  }

  auto& booking = it->second;
  if (booking.user_id != requester_user_id) {
    throw ServiceError(
        ServiceErrorKind::kForbidden,
        "forbidden",
        "You can only cancel your own bookings");
  }
  if (booking.status == BookingStatus::kCancelled) {
    throw ServiceError(
        ServiceErrorKind::kConflict,
        "booking_already_cancelled",
        "Booking has already been cancelled");
  }

  booking.status = BookingStatus::kCancelled;
}

}
