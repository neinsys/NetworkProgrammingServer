
create table chat(
	idx int not null auto_increment primary key,
	user_idx int not null,
    group_idx int not null,
    chat_type char(4),
    content longtext,
    chat_time datetime not null,
    foreign key (user_idx) references user(idx) on delete cascade,
    foreign key (group_idx) references project_group(idx) on delete cascade
);