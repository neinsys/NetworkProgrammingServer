create table chatroom(
	idx int NOT NULL AUTO_INCREMENT PRIMARY KEY,
    group_idx int NOT NULL,
    name varchar(25),
    foreign key (group_idx) references project_group(idx) on delete cascade
);

create table chat(
	idx int not null auto_increment primary key,
    chatroom_idx int not null,
    chat_type char(4),
    content longtext,
    foreign key (chatroom_idx) references chatroom(idx) on delete cascade
);