TOPIC 41: Developing a C++ System for Managing Courier Shipments

Create a C++ console application to automate the logistics processes at a courier company - accepting shipments, tracking their status, calculating shipping costs based on weight and service type, and assigning them to couriers or offices. The application must demonstrate an OOP architecture using lambda expressions, design patterns (Factory Pattern, Observer Pattern, Command Pattern), a polymorphic hierarchy of shipments, work with STL containers (std::queue, std::unordered_map), exception handling, and file storage.

The system must support the following key modules and data structures:

Package: a base abstract class and polymorphic subclasses (StandardPackage, ExpressPackage, FragilePackage) that override the base class's virtual methods with specific logic for calculating the delivery fee.

PackageFactory: a design pattern for the centralized creation of Package-type objects based on the requested service type.

LogisticsManager: a management class that uses queues to sort shipments for delivery, track status (accepted, in transit, delivered), and maintain a history of canceled operations.

Requirements:

Functionality: The application must provide the ability to accept new shipments with a unique tracking number (Tracking ID), automatically calculate the cost based on weight, volume, and insurance, change the shipment status, search by tracking number, generate reports on undelivered shipments, and maintain a history of logistics operations.

Software design and language constructs used:Use of lambda expressions for flexible filtering, sorting, and processing of shipments.

Mandatory implementation of the Factory Pattern to encapsulate the logic for creating different types of shipments.

Mandatory implementation of the Observer Pattern to automatically notify the dispatch module when the status of express or fragile shipments changes.

Mandatory implementation of the Command Pattern to support undo and redo operations when rerouting or modifying shipments.

A pure polymorphic class hierarchy with a virtual destructor and virtual methods for calculations.

Data Structures (STL): Use of the STL container `std::queue` to organize the logistics queue of shipments for processing and delivery.

Use of std::unordered_map to search for shipments by their unique tracking number.

OOP implementation, encapsulation, and smart pointers: Member variables must be private with public access methods (getters/setters) and at least two constructors for each class. Safe management of dynamic polymorphic objects using smart pointers (std::unique_ptr<Package> or std::shared_ptr<Package>).

Exception handling and validation: Catch exceptions using a custom error class. Validation of weight and dimensions to protect against invalid or negative values.

Operators and file processing: Redefining the << operator (operator<<) for formatted output of shipment data (waybill/label). Permanent storage of the entire logistics history in text/CSV files.

UML Diagram: The system architecture, including class hierarchies, relationships between objects, applied design patterns, and the exception hierarchy, should be represented using a UML class diagram.
