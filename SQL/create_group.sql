create table project_group (
    idx int NOT NULL AUTO_INCREMENT PRIMARY KEY,
    name varchar(25) default NULL,
    owner int NOT NULL,
    FOREIGN KEY (owner) references user(idx) on delete cascade
);

create table join_group (
    user_idx int NOT NULL,
    group_idx int NOT NULL,
    FOREIGN KEY (user_idx) references user(idx) on delete cascade,
    FOREIGN KEY (group_idx) references project_group(idx) on delete cascade
);
