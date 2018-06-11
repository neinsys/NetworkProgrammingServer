
create table attendance (
    user_idx int NOT NULL,
    group_idx int NOT NULL,
    attendance_date date NOT NULL unique,
    FOREIGN KEY (user_idx) references user(idx) on delete cascade,
    FOREIGN KEY (group_idx) references project_group(idx) on delete cascade
);
