
create table todo(
    user_idx int NOT NULL,
    achieve boolean NOT NULL default false,
    deadline date NOT NULL,
    detail varchar(256) NOT NULL,
    FOREIGN KEY (user_idx) references user(idx) on delete cascade
)