
create table todo(
    user_idx int NOT NULL,
    achieve boolean NOT NULL default false,
    deadline date NOT NULL,
    detail varchar(256) NOT NULL,
    FOREIGN KEY (user_idx) references user(idx) on delete cascade
)

create table progress(
    idx int NOT NULL AUTO_INCREMENT primary key,
    user_idx int NOT NULL,
    number int NOT NULL,
    FOREIGN KEY (user_idx) references user(idx) on delete cascade

)