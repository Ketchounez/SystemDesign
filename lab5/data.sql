INSERT INTO users (login, name, surname, email, password_hash) VALUES
('user01', 'Ivan', 'Petrov', 'user01@mail.test', 'h1'),
('user02', 'Petr', 'Ivanov', 'user02@mail.test', 'h2'),
('user03', 'Anna', 'Sidorova', 'user03@mail.test', 'h3'),
('user04', 'Olga', 'Romanova', 'user04@mail.test', 'h4'),
('user05', 'Sergey', 'Smirnov', 'user05@mail.test', 'h5'),
('user06', 'Maria', 'Kuznetsova', 'user06@mail.test', 'h6'),
('user07', 'Denis', 'Fedorov', 'user07@mail.test', 'h7'),
('user08', 'Nikita', 'Volkov', 'user08@mail.test', 'h8'),
('user09', 'Daria', 'Orlova', 'user09@mail.test', 'h9'),
('user10', 'Alexey', 'Morozov', 'user10@mail.test', 'h10')
ON CONFLICT DO NOTHING;

INSERT INTO hotels (name, city, address, description, created_by_user_id) VALUES
('City Inn', 'Moscow', 'Tverskaya 1', 'Business hotel', 1),
('River View', 'Moscow', 'Arbat 10', 'Near river', 2),
('Nevsky Place', 'Saint Petersburg', 'Nevsky 23', 'Center hotel', 3),
('Sea Breeze', 'Sochi', 'Kurortny 7', 'Sea side', 4),
('Ural Stay', 'Ekaterinburg', 'Lenina 45', 'Modern rooms', 5),
('Kazan Palace', 'Kazan', 'Baumana 2', 'Historic style', 6),
('Siberia Lodge', 'Novosibirsk', 'Sovetskaya 12', 'Family hotel', 7),
('Volga Rooms', 'Samara', 'Molodogvardeyskaya 3', 'Budget option', 8),
('Don Comfort', 'Rostov-on-Don', 'Pushkinskaya 21', 'Comfort class', 9),
('Baltic House', 'Kaliningrad', 'Mira 5', 'Old town', 10)
ON CONFLICT DO NOTHING;
