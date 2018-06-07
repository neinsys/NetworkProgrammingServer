create table user (
    idx int NOT NULL AUTO_INCREMENT PRIMARY KEY,
    name varchar(25) default NULL,
    ID varchar(20),
    password varchar(20)
);

create table token(
    token char(50) NOT NULL,
    user_idx int NOT NULL,
    FOREIGN KEY (user_idx) references user(idx) on delete cascade
)