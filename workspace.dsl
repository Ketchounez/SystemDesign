
workspace "Система бронирования отелей" "Архитектура системы бронирования отелей (вариант 13)" {

    model {

        user = person "Пользователь" "Пользователь системы бронирования отелей"

        emailSystem = softwareSystem "Email Service" "Внешний сервис отправки email уведомлений"

        bookingSystem = softwareSystem "Система бронирования отелей" "Система поиска отелей и управления бронированиями" {

            webapp = container "Web Application" "Веб-интерфейс системы бронирования" "React"

            api = container "API Application" "Backend API с бизнес-логикой системы" "Java Spring Boot REST API"

            bookingService = container "Booking Service" "Сервис управления бронированиями" "Java REST Service"

            db = container "Database" "Хранит пользователей, отели и бронирования" "PostgreSQL"

            webapp -> api "Отправляет API запросы" "HTTPS/REST"

            api -> bookingService "Управляет бронированиями" "REST"

            api -> db "Читает и записывает данные" "JDBC"

            bookingService -> db "Сохраняет данные бронирования" "JDBC"

            bookingService -> emailSystem "Отправляет уведомления о бронировании" "SMTP API"
        }

        user -> webapp "Использует систему" "HTTPS"

    }

    views {

        systemContext bookingSystem "system-context-diagram" {
            include *
            autolayout lr
        }

        container bookingSystem "container-diagram" {
            include *
            autolayout lr
        }

        dynamic bookingSystem "dynamic-diagram" {

            user -> webapp "Создает бронирование"

            webapp -> api "POST /bookings"

            api -> bookingService "Создание бронирования"

            bookingService -> db "Сохранение бронирования"

            bookingService -> emailSystem "Отправка подтверждения"

            api -> webapp "Возврат результата"

            autolayout lr
        }

        theme default
    }

}
