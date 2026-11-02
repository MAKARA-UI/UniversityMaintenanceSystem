CREATE TABLE IF NOT EXISTS accounts (
    account_id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL COLLATE NOCASE UNIQUE,
    password_salt TEXT NOT NULL,
    password_hash TEXT NOT NULL,
    role TEXT NOT NULL DEFAULT 'Reporter'
        CHECK (role IN ('Administrator', 'Facilities manager', 'Technician', 'Reporter')),
    is_active INTEGER NOT NULL DEFAULT 1 CHECK (is_active IN (0, 1)),
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS fault_reports (
    fault_id INTEGER PRIMARY KEY AUTOINCREMENT,
    campus TEXT NOT NULL DEFAULT 'Main Campus'
        CHECK (campus IN ('Main Campus', 'MP Campus')),
    building TEXT NOT NULL CHECK (length(trim(building)) BETWEEN 1 AND 100),
    room TEXT NOT NULL CHECK (length(trim(room)) BETWEEN 1 AND 50),
    category TEXT NOT NULL CHECK (category IN (
        'Electrical', 'Plumbing', 'Furniture', 'IT / AV', 'Safety', 'Other'
    )),
    description TEXT NOT NULL CHECK (length(trim(description)) BETWEEN 10 AND 1000),
    urgency TEXT NOT NULL CHECK (urgency IN ('Emergency', 'Urgent', 'Normal', 'Low')),
    priority TEXT NOT NULL CHECK (priority IN ('Critical', 'High', 'Medium', 'Low')),
    status TEXT NOT NULL DEFAULT 'Submitted'
        CHECK (status IN ('Submitted', 'Assigned', 'In Progress',
                          'Awaiting Parts', 'Resolved', 'Closed')),
    reported_at TEXT NOT NULL,
    reported_by TEXT NOT NULL DEFAULT 'legacy'
);

CREATE TABLE IF NOT EXISTS work_orders (
    work_order_id INTEGER PRIMARY KEY AUTOINCREMENT,
    fault_id INTEGER NOT NULL UNIQUE REFERENCES fault_reports(fault_id),
    technician_username TEXT NOT NULL,
    assigned_by TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'Assigned'
        CHECK (status IN ('Assigned', 'Accepted', 'In Progress', 'Awaiting Parts',
                          'Completed - Review', 'Closed')),
    work_performed TEXT NOT NULL DEFAULT '',
    parts_requested TEXT NOT NULL DEFAULT '',
    parts_used TEXT NOT NULL DEFAULT '',
    assigned_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS campus_rooms (
    room_id INTEGER PRIMARY KEY AUTOINCREMENT,
    campus TEXT NOT NULL CHECK (campus IN ('Main Campus', 'MP Campus')),
    building TEXT NOT NULL CHECK (length(trim(building)) BETWEEN 1 AND 100),
    room TEXT NOT NULL CHECK (length(trim(room)) BETWEEN 1 AND 50),
    UNIQUE (campus, building, room)
);

