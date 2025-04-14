This project implements a simple Healthcare Management System for managing doctor and appointment records using file-based data storage and indexing techniques. It was developed as part of an academic assignment for the File Management and Processing course.

The system supports adding, updating, deleting, and searching doctors and appointments. It also includes the creation and maintenance of primary and secondary indexes using binary search and linked list techniques.

Features
Add, delete, and update doctor and appointment records.

Search functionality using:

Primary index on Doctor ID and Appointment ID.

Secondary index on Doctor Name and Doctor ID (for Appointments).

Indexes are kept sorted to support binary search.

Secondary indexes are linked to primary indexes via linked lists.

Efficient record reuse using an Avail List.

Delimited fields and length indicator records for structured file storage.
Technologies Used
C++

File handling and I/O

Basic data structures (linked lists, vectors)

Sorting and binary search algorithms
