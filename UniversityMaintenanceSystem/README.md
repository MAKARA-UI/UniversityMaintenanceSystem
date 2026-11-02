# University Facilities Fault Reporting and Maintenance Management System

A Linux desktop application for recording university facility faults and supporting their maintenance workflow. It uses C++17, Qt 6 Widgets, SQLite, and CMake.

## Current application increment

The application has role-specific dashboards for Administrator, Facilities Manager, Technician, and Reporter:

- Reporters select Main Campus or MP Campus, submit fault details, and view the status of their own reports.
- Facilities managers review and prioritise the fault queue, assign active technicians, monitor work orders, close completed work, and view maintenance counts.
- Technicians see their assigned work, accept or start jobs, record work performed, update progress, and request or record spare parts against a job.
- Administrators create accounts, assign roles, activate or deactivate accounts, register campus buildings and rooms, view technicians, and set default campus and urgency values.
- Successful actions display text-only confirmation dialogs and refresh the related views.
- Dashboard text and controls use larger sizing and role-specific sidebar styling.

The database file is stored in the current user's Qt application-data directory. The schema is in `database/schema.sql` and CMake copies it next to the built application. The parts feature records requests and usage per work order; it does not provide a stock quantity catalogue. Role choice is available during sign-up for this class demonstration and should be restricted to an administrator in a deployed system. Accounts are local to this installation and are not connected to university identity services.

## Build and run on Ubuntu or Ubuntu in WSL

Open this folder through VS Code connected to WSL: Ubuntu-24.04. In the Ubuntu terminal, run:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/university_maintenance_system
```

Required packages are CMake, Ninja, a C++17 compiler, Qt 6 Widgets, the Qt SQLite driver, and OpenSSL development files. On Ubuntu, install `build-essential`, `cmake`, `ninja-build`, `qt6-base-dev`, `libqt6sql6-sqlite`, and `libssl-dev`.

## Current design

- `src/domain` contains the fault data type and business rules.
- `src/data` contains the SQLite repository.
- `src/data/AccountRepository.*` handles local account creation and sign-in.
- `src/ui/MainWindow.cpp` and `MainWindow.h` contain the shared dashboard window, role navigation, and home page.
- `src/ui/dashboards/` groups dashboard implementation by role, with shared fault-list and work-order table code alongside them. See its README for the file map.
- `src/ui/UiHelpers.h` contains common labels and text-only message dialogs.
- `database` contains the initial SQLite schema.
- `resources/styles/` contains shared component styles, plus a `dashboards/` subfolder with one QSS file per role. See `resources/styles/README.md` for the map.
- `resources/resources.qrc` bundles the style sheets into the Linux executable.

The UI delegates validation and priority decisions to the domain rules. Database writes use bound parameters so user-entered text is treated as data.
