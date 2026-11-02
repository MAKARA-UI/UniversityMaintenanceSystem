# Interface styles

Qt style sheets are split by interface area so each part can be edited separately:

- `base.qss` sets the shared palette and default font.
- `authentication.qss` styles sign-in and account creation.
- `sidebar.qss` styles navigation, account details, and logout on every dashboard.
- `dashboard.qss` styles dashboard headings, summary cards, and information panels.
- `dashboards/AdministratorDashboard.qss` styles only Administrator dashboard accents.
- `dashboards/FacilitiesManagerDashboard.qss` styles only Facilities Manager dashboard accents.
- `dashboards/TechnicianDashboard.qss` styles only Technician dashboard accents.
- `dashboards/ReporterDashboard.qss` styles only Reporter dashboard accents.
- `forms.qss` styles input fields, buttons, and keyboard focus.
- `tables.qss` styles tables, headings, and scrollbars.
- `dialogs.qss` styles confirmation and message dialogs.

The application loads them in this order from `src/main.cpp`. Each dashboard's main window area has a role-specific object name, which scopes the files under `dashboards/` to that role. They are embedded by `resources/resources.qrc`, so rebuild the application after changing a QSS file.
