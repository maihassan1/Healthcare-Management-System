#include <bits/stdc++.h>
using namespace std;

#define docav "avalListDoc.txt"
#define appav "avalListApp.txt"

struct Doctor {
    string doctorId;
    string doctorName;
    string address;
};

struct Appointment {
    string appointmentId;
    string appointmentDate;
    string doctorId;
};

struct IndexEntry {
    string key;
    streampos position;
};
struct AvailNode {
    streampos offset;
    size_t size;
    AvailNode* next;
};

class Healthcare {
private:
    string doctorFilePath = "doctors.txt";
    string appointmentFilePath = "appointments.txt";
    string drPrimaryIndexFile = "primary_index_doctors.txt";
    string apPrimaryIndexFile = "primary_index_appointments.txt";
    string nameSecondaryIndexFile = "name_secondary_index.txt";
    string invertedListFile = "doctor_inverted_list.txt";
    string appointmentSecondaryIndexFile = "doctor_secondary_index.txt";
    string appointmentInvertedListFile = "appointment_inverted_list.txt";
    string DravailListFile ="availListDoc.txt";
    string ApavailListFile ="avalListApp.txt";
    AvailNode* doctorAvailListHead = nullptr;
    AvailNode* appointmentAvailListHead = nullptr;

    vector<IndexEntry> drPrimaryIndex;

    map<int,int> availList;
    vector<IndexEntry> appointmentIndex;

    void buildPrimaryIndex(const string& filePath, string IndexFile, vector<IndexEntry>& index, map<int, int>& availList) {
        ifstream dataStream(filePath, ios::in);
        if (!dataStream.is_open()) {
            cerr << "Error: Unable to open file " << filePath << " for reading." << endl;
            return;
        }

        string line;
        int charOffset = 0;
        index.clear();
        availList.clear();

        while (getline(dataStream, line)) {
            if (line.empty() || line[0] == '*') {
                // If line is marked deleted or empty, update offset and continue
                int recordLength = line.size();
                charOffset += recordLength + 1;
                if (!line.empty()) {
                    availList[charOffset - (recordLength + 1)] = recordLength; // Map offset to record length
                }
                continue;
            }

            size_t delimiterPos = line.find('|');
            if (delimiterPos == string::npos) {
                charOffset += line.size() + 1;
                continue;
            }

            string lengthStr = line.substr(0, delimiterPos);
            int recordLength = stoi(lengthStr);
            string record = line.substr(delimiterPos + 1);

            size_t keyDelimiterPos = record.find('|');
            string key = record.substr(0, keyDelimiterPos);
            index.push_back({key, charOffset});

            charOffset += recordLength + 1;
        }

        dataStream.close();

        ofstream indexStream(IndexFile, ios::trunc);
        for (const auto& entry : index) {
            indexStream << entry.key << "->" << entry.position << endl;
        }

        indexStream.close();
        cout << "Primary index and AVAIL list rebuilt successfully.\n";
    }



    int findAppointmentPosition(const string& appointmentId) {
        auto it = lower_bound(appointmentIndex.begin(), appointmentIndex.end(), appointmentId,
                              [](const IndexEntry& entry, const string& id) {
                                  return entry.key < id;
                              });
        if (it != appointmentIndex.end() && it->key == appointmentId) {
            return it->position;
        }
        return -1; // Appointment not found
    }
    void buildSecondaryIndex() {
        ifstream dataStream(doctorFilePath, ios::in);
        if (!dataStream.is_open()) {
            cerr << "Error: Unable to open doctors file for reading." << endl;
            return;
        }

        string line;
        unordered_map<string, vector<int>> nameToIndices;

        vector<string> doctorList;
        while (getline(dataStream, line)) {
            size_t delimiterPos = line.find('|');
            if (delimiterPos == string::npos) continue;

            string lengthStr = line.substr(0, delimiterPos);

            string record = line.substr(delimiterPos + 1);

            if (line[0] != '*') {
                stringstream ss(record);
                string doctorId, doctorName;
                getline(ss, doctorId, '|');
                getline(ss, doctorName, '|');

                doctorList.push_back(doctorId);


                nameToIndices[doctorName].push_back(doctorList.size() - 1);
            }

        }

        dataStream.close();

        ofstream secIndexStream(nameSecondaryIndexFile, ios::trunc);
        ofstream invertedListStream(invertedListFile, ios::trunc);

        int invertedListPos = 0;
        for (const auto& [name, indices] : nameToIndices) {

            secIndexStream << name << "->" << invertedListPos << endl;

            for (size_t i = 0; i < indices.size(); ++i) {
                string doctorId = doctorList[indices[i]];
                int nextPointer = (i < indices.size() - 1) ? invertedListPos + 1 : -1;

                invertedListStream << doctorId << "->" << nextPointer << endl;
                invertedListPos++;
            }
        }

        secIndexStream.close();
        invertedListStream.close();
        cout << "Doctor Name Secondary index and inverted list saved." << endl;
    }



    void buildAppointmentSecondaryIndex() {
        ifstream dataStream(appointmentFilePath, ios::in);
        if (!dataStream.is_open()) {
            cerr << "Error: Unable to open appointments file for reading." << endl;
            return;
        }

        string line;
        vector<string> appointments;
        unordered_map<string, vector<int>> doctorToAppointments;

        while (getline(dataStream, line)) {
            // Skip deleted records (those starting with '*')
            if (line[0] == '*') continue;

            size_t delimiterPos = line.find('|');
            if (delimiterPos == string::npos) continue;

            string record = line.substr(delimiterPos + 1);

            stringstream ss(record);
            string appointmentId, appointmentDate, doctorId;
            getline(ss, appointmentId, '|');
            getline(ss, appointmentDate, '|');
            getline(ss, doctorId, '|');

            appointments.push_back(appointmentId);
            doctorToAppointments[doctorId].push_back(appointments.size() - 1);
        }

        dataStream.close();

        vector<pair<string, int>> secondaryIndexEntries;
        int invertedListPos = 0;

        // Only include doctors that have appointments and skip deleted ones
        for (const auto &[doctorId, indices]: doctorToAppointments) {
            if (!indices.empty()) {
                secondaryIndexEntries.push_back({doctorId, invertedListPos});
                invertedListPos += indices.size();
            }
        }

        sort(secondaryIndexEntries.begin(), secondaryIndexEntries.end(),
             [](const pair<string, int> &a, const pair<string, int> &b) {
                 return a.first < b.first;
             });

        ofstream secIndexStream(appointmentSecondaryIndexFile, ios::trunc);
        ofstream invertedListStream(appointmentInvertedListFile, ios::trunc);

        invertedListPos = 0;
        for (const auto &[doctorId, startPos]: secondaryIndexEntries) {
            secIndexStream << doctorId << "->" << startPos << endl;
        }

        for (const auto &[doctorId, indices]: doctorToAppointments) {
            for (size_t i = 0; i < indices.size(); ++i) {
                const auto &appointment = appointments[indices[i]];

                // Ensure no deleted appointments are included in the inverted list
                if (appointment[0] != '*') {
                    int nextPointer = (i < indices.size() - 1) ? invertedListPos + 1 : -1;
                    invertedListStream << appointment << "->" << nextPointer << endl;
                    invertedListPos++;
                }
            }
        }

        secIndexStream.close();
        invertedListStream.close();
        cout << "Sorted Appointment Secondary index and inverted list saved." << endl;
    }
//    AvailNode* doctorAvailListHead = nullptr;
//    AvailNode* appointmentAvailListHead = nullptr;

    void insertAvailNode(AvailNode*& head, streampos offset, size_t size) {
        AvailNode* newNode = new AvailNode{offset, size, nullptr};
        if (!head || head->size > size) { // Insert at the head
            newNode->next = head;
            head = newNode;
            return;
        }

        AvailNode* current = head;
        while (current->next && current->next->size < size) {
            current = current->next;
        }
        newNode->next = current->next;
        current->next = newNode;
    }

    AvailNode* findAndRemoveBestFit(AvailNode*& head, size_t recordLength) {
        AvailNode** current = &head;
        AvailNode* bestFit = *current;
        AvailNode** bestFitPrev = current;

        while (*current) {
            if ((*current)->size >= recordLength) {
                if ( (*current)->size < bestFit->size) {
                    bestFit = *current;
                    bestFitPrev = current;
                    break;
                }
            }
            current = &((*current)->next);
        }

        if (bestFit) {
            *bestFitPrev = bestFit->next;
        }
        return bestFit;
    }

    void writeAvailListToFile(const string& filename) {
        fstream file(filename, ios::out );
        if (!file.is_open()) {
            cerr << "Error: Could not open the file to write avail list.\n";
            return;
        }

        AvailNode* current = doctorAvailListHead;
        while (current) {
            file<<current->offset<<"|"<<current->size<<'\n';
            current = current->next;
        }

        file.close();
    }

    void loadAvailList(const string& filename,AvailNode* node) {
        fstream file(filename, ios::in | ios::out);
        if (!file.is_open()) {
            fstream file(filename, ios::out);
            cerr << "Could not open file, created new one.\n";
            return;
        }

        string line;
        while (getline(file, line)) {
            // Assuming the line contains offset and size, separated by a delimiter
            size_t delimiterPos = line.find('|');
            if (delimiterPos != string::npos) {
                AvailNode* newNode = new AvailNode;

                // Extract offset and size from the line
                newNode->offset = stoll(line.substr(0, delimiterPos));
                newNode->size = stoi(line.substr(delimiterPos + 1));

                newNode->next = nullptr;
                insertAvailNode(node, newNode->offset, newNode->size);
            }
        }

        file.close();
    }


    int binarySearchPrimaryIdx(const string& doctorId) {
        auto it = lower_bound(drPrimaryIndex.begin(), drPrimaryIndex.end(), doctorId,
                              [](const IndexEntry& entry, const string& id) {
                                  return entry.key < id;
                              });
        if (it != drPrimaryIndex.end() && it->key == doctorId) {
            return it->position;
        }
        return -1; // Doctor not found
    }
    void rebuildIndices() {
        buildPrimaryIndex(doctorFilePath,drPrimaryIndexFile, drPrimaryIndex, availList);
        buildPrimaryIndex(appointmentFilePath,apPrimaryIndexFile, appointmentIndex, availList);
        buildAppointmentSecondaryIndex();
        buildSecondaryIndex();
    }

public:
    Healthcare(){
        loadAvailList(docav,doctorAvailListHead);
        loadAvailList(appav,appointmentAvailListHead);
        rebuildIndices();
    }
    // Add Doctor
    void add_doctor() {
        Doctor doc;
        cout << "Please enter the following doctor info:\n";
        cout << "ID: ";
        cin >> doc.doctorId;

        // Check if the doctor ID already exists
        if (binarySearchPrimaryIdx(doc.doctorId) != -1) {
            cout << "Doctor ID already exists.\n";
            return;
        }

        cout << "Name: ";
        cin.ignore();
        getline(cin, doc.doctorName);

        cout << "Address: ";
        getline(cin, doc.address);

        // Create the record
        stringstream recordStream;
        recordStream << doc.doctorId << "|"
            << doc.doctorName << "|"
                     << doc.address;
        string recordBody = recordStream.str();

        // Add total length indicator
        size_t recordLength = recordBody.size() + to_string(recordBody.size()).size() + 1;
        string finalRecord = to_string(recordLength) + "|" + recordBody;

        fstream file(doctorFilePath, ios::in | ios::out);
        if (!file.is_open()) {
            cout << "Error opening doctor file.\n";
            return;
        }

        streampos offset = -1;

        // Use space from the availability list if possible
        if (doctorAvailListHead) {
            AvailNode* bestFit = findAndRemoveBestFit(doctorAvailListHead, recordLength);
            if (bestFit) {
                offset = bestFit->offset;
                writeAvailListToFile(DravailListFile);
            } else {
                file.seekp(0, ios::end);
                offset = file.tellp();
            }
        } else {
            file.seekp(0, ios::end);
            offset = file.tellp();
        }

        // Write the record to the file at the determined offset
        file.seekp(offset);
        file << finalRecord << endl;
        file.close();

        // Update the primary index by rebuilding it
  rebuildIndices();

        cout << "Doctor added successfully and primary index updated.\n";
    }



    void searchDoctor(string doctorId) {


        // Find the position of the record in the file
        int position = binarySearchPrimaryIdx(doctorId);
        if (position == -1) {
            cout << "Doctor not found.\n";
            return;
        }


        // Open the file for reading
        ifstream file(doctorFilePath, ios::in);
        if (!file.is_open()) {
            cout << "Failed to open file.\n";
            return;
        }
        string line;
        Doctor doctor;

        while (getline(file, line)) {
            size_t delimiterPos = line.find('|');
            if (delimiterPos == string::npos) continue;

            string lengthStr = line.substr(0, delimiterPos);
            string record = line.substr(delimiterPos + 1);

            stringstream ss(record);
            string recordDoctorId;
            getline(ss, recordDoctorId, '|');

            if (recordDoctorId == doctorId && line[0] != '*') {

                doctor.doctorId = recordDoctorId;
                getline(ss, doctor.doctorName, '|');
                getline(ss, doctor.address, '|');
            }
        }



        // Output the parsed details
        cout << "Doctor ID: " << doctor.doctorId << "\n";
        cout << "Name: " << doctor.doctorName << "\n";
        cout << "Address: " << doctor.address << "\n";

        // Close the file
        file.close();
    }





    // Add Appointment
    void addAppointment() {
        Appointment appointment;

        cout << "Enter Appointment ID: ";
        cin >> appointment.appointmentId;

        int position = findAppointmentPosition(appointment.appointmentId);
        if (position != -1) {
            cout << "Error: Appointment with this ID already exists!\n";
            return;
        }

        cout << "Enter Appointment Date: ";
        cin.ignore();  // Clear input buffer
        getline(cin, appointment.appointmentDate);

        cout << "Enter Doctor ID for the Appointment: ";
        cin >> appointment.doctorId;

        if (binarySearchPrimaryIdx(appointment.doctorId) == -1) {
            cout << "Doctor ID not found! Please add the doctor first.\n";
            return;
        }

        ofstream file(appointmentFilePath, ios::in | ios::out | ios::app);
        if (!file.is_open()) {
            cerr << "Error: Unable to open appointments file for adding.\n";
            return;
        }

        int positionToWrite = -1;
        stringstream ss;
        ss << appointment.appointmentId << "|" << appointment.appointmentDate << "|" << appointment.doctorId << "|";
        string record = ss.str();
        int recordLength = record.length();

        // Check the availability of space in the avail list
        auto it = availList.begin();
        while (it != availList.end()) {
            if (it->second >= recordLength) {  // Check if space is sufficient
                positionToWrite = it->first;
                availList.erase(it);  // Remove the used space
                break;
            }
            ++it;
        }

        if (positionToWrite == -1) {
            positionToWrite = file.tellp();  // If no space is sufficient, append at the end
        }

        file.seekp(positionToWrite, ios::beg);  // Write at the correct position
        file << recordLength << "|" << record << endl;

        file.close();
        rebuildIndices();  // Rebuild indices after adding an appointment
        cout << "Appointment added successfully!\n";
    }


    // Search Appointment by ID
    void searchAppointment(string appointmentId) {


        int position = findAppointmentPosition(appointmentId);
        if (position == -1) {
            cout << "Appointment not found.\n";
            return;
        }

        ifstream file(appointmentFilePath);
//        file.seekg(position, ios::beg);  // Move to the correct position in the file

        string line;
        Appointment appointment;


        while (getline(file, line)) {
            size_t delimiterPos = line.find('|');
            if (delimiterPos == string::npos) continue;

            string lengthStr = line.substr(0, delimiterPos);
            string record = line.substr(delimiterPos + 1);

            stringstream ss(record);
            string recordApId;
            getline(ss, recordApId, '|');

            if (recordApId == appointmentId && line[0] != '*') {
                appointment.appointmentId = recordApId;
                getline(ss, appointment.appointmentDate, '|');
                getline(ss, appointment.doctorId, '|');
            }
        }



        cout << "Appointment ID: " << appointment.appointmentId << "\n";
        cout << "Date: " << appointment.appointmentDate << "\n";
        cout << "Doctor ID: " << appointment.doctorId << "\n";
    }
    void deleteDoctor() {

        string doctorId;
        cout<< "enter doctor ID to be deleted:";
        cin>>doctorId;
        ifstream dataStream(doctorFilePath, ios::in);
        if (!dataStream.is_open()) {
            cerr << "Error: Unable to open doctors file for deletion." << endl;
            return;
        }

        // Delete related appointments first
        deleteRelatedAppointments(doctorId);  // This function will handle deleting appointments

        // Proceed with deleting the doctor
        ofstream tempStream("temp.txt", ios::trunc);
        if (!tempStream.is_open()) {
            cerr << "Error: Unable to open temporary file for writing." << endl;
            dataStream.close();
            return;
        }

        string line;
        bool found = false;

        while (getline(dataStream, line)) {
            size_t delimiterPos = line.find('|');
            if (delimiterPos == string::npos) continue;

            string lengthStr = line.substr(0, delimiterPos);
            string record = line.substr(delimiterPos + 1);

            stringstream ss(record);
            string recordDoctorId;
            getline(ss, recordDoctorId, '|');

            if (recordDoctorId == doctorId && line[0] != '*') {
                found = true;
                tempStream << "*" << line.substr(1) << endl; // Mark the doctor as deleted
            }
            else {
                tempStream << line << endl; // Write the record as is
            }
        }

        dataStream.close();
        tempStream.close();

        if (found) {
            // Replace the original file with the temp file
            remove(doctorFilePath.c_str());
            rename("temp.txt", doctorFilePath.c_str());

            // Rebuild all indexes after deletion
            rebuildIndices();

            cout << "Doctor with ID " << doctorId << " has been deleted successfully." << endl;
        }
        else {
            remove("temp.txt"); // Clean up temp file
            cout << "Doctor with ID " << doctorId << " not found." << endl;
        }
    }

    void deleteRelatedAppointments(const string& doctorId) {
        ifstream dataStream(appointmentFilePath, ios::in);
        if (!dataStream.is_open()) {
            cerr << "Error: Unable to open appointments file for deletion." << endl;
            return;
        }

        ofstream tempStream("tempAppointments.txt", ios::trunc);
        if (!tempStream.is_open()) {
            cerr << "Error: Unable to open temporary appointment file for writing." << endl;
            dataStream.close();
            return;
        }

        string line;
        bool appointmentFound = false;

        while (getline(dataStream, line)) {
            size_t delimiterPos = line.find('|');
            if (delimiterPos == string::npos) continue;

            string record = line.substr(delimiterPos + 1);

            stringstream ss(record);
            string appointmentId, appointmentDate, doctorIdInAppointment;

            // Correctly extract appointmentId, appointmentDate, and doctorId
            getline(ss, appointmentId, '|');    // Extract appointment ID
            getline(ss, appointmentDate, '|');  // Extract appointment Date
            getline(ss, doctorIdInAppointment, '|');  // Extract doctor ID

            // Debugging output: Print doctor IDs to verify if they're matching
            cout << "Doctor in Appointment: '" << doctorIdInAppointment << "' | Doctor to delete: '" << doctorId << "'" << endl;

            if (doctorIdInAppointment == doctorId && line[0] != '*') {
                appointmentFound = true;
                tempStream << "*" << line.substr(1) << endl; // Mark the appointment as deleted
            }
            else {
                tempStream << line << endl; // Write the record as is
            }
        }

        dataStream.close();
        tempStream.close();


        if (appointmentFound) {
            remove(appointmentFilePath.c_str());
            rename("tempAppointments.txt", appointmentFilePath.c_str());
            cout << "Appointments related to Doctor ID " << doctorId << " have been deleted." << endl;
        } else {
            cout << "No appointments found for Doctor ID " << doctorId << " to delete." << endl;
        }
    }
    void updateDoctor() {
        string doctorId;
        cout << "Enter Doctor ID to update: ";
        cin >> doctorId;

        int position = binarySearchPrimaryIdx(doctorId);
        if (position == -1) {
            cout << "Doctor not found.\n";
            return;
        }


        searchDoctor(doctorId);
        Doctor doctor;
        // Now prompt for new information
        doctor.doctorId = doctorId;
        cout << "Enter new Doctor Name: ";
        cin.ignore();  // Clear input buffer
        getline(cin, doctor.doctorName);

        cout << "Enter new Doctor Address: ";
        getline(cin, doctor.address);

        ofstream tempStream("temp_doctors.txt", ios::trunc);
        if (!tempStream.is_open()) {
            cerr << "Error: Unable to open temporary file for writing.\n";

            return;
        }

        // Rewrite the file with the updated doctor information
        string line;
        ifstream originalFile(doctorFilePath);
        string updatedLine;
        while (getline(originalFile, line)) {
            if (line.find(doctorId) != string::npos && line[0] != '*') {
                // If the doctor ID matches, update the record
                stringstream ss;
                ss << doctor.doctorId << "|" << doctor.doctorName << "|" << doctor.address ;
                updatedLine = ss.str();


                // Add total length indicator
                size_t recordLength = updatedLine.size() + to_string(updatedLine.size()).size() + 1;
                string finalRecord = to_string(recordLength) + "|" + updatedLine;
                tempStream << finalRecord << endl;
            } else {
                // Write the record as is if no match
                tempStream << line << endl;
            }
        }

        originalFile.close();
        tempStream.close();

        // Replace the original file with the temp file
        remove(doctorFilePath.c_str());
        rename("temp_doctors.txt", doctorFilePath.c_str());

        rebuildIndices();  // Rebuild indices after the update
        cout << "Doctor updated successfully!\n";
    }

// Update Appointment Information
    void updateAppointment() {
        string appointmentId;
        cout << "Enter Appointment ID to update: ";
        cin >> appointmentId;

        int position = findAppointmentPosition(appointmentId);
        if (position == -1) {
            cout << "Appointment not found.\n";
            return;
        }
        searchAppointment(appointmentId);
        Appointment appointment;


        appointment.appointmentId=appointmentId;
        // Now prompt for new information
        cout << "Enter new Appointment Date (yyyy-mm-dd): ";
        cin.ignore();  // Clear input buffer
        getline(cin, appointment.appointmentDate);

        cout << "Enter new Doctor ID for the Appointment: ";
        cin >> appointment.doctorId;

        if (binarySearchPrimaryIdx(appointment.doctorId) == -1) {
            cout << "Doctor ID not found! Please ensure the doctor exists.\n";
            return;
        }

        ofstream tempStream("temp_appointments.txt", ios::trunc);
        if (!tempStream.is_open()) {
            cerr << "Error: Unable to open temporary file for writing.\n";
            return;
        }

        string line;


        ifstream originalFile(appointmentFilePath);
        string updatedLine;
        while (getline(originalFile, line)) {
            if (line.find(appointmentId) != string::npos && line[0] != '*') {
                // If the appointment ID matches, update the record
                stringstream ss;
                ss << appointment.appointmentId << "|" << appointment.appointmentDate << "|" << appointment.doctorId << "|";
                updatedLine = ss.str();
                size_t recordLength = updatedLine.size() + to_string(updatedLine.size()).size() + 1;
                string finalRecord = to_string(recordLength) + "|" + updatedLine;
                tempStream << finalRecord << endl;

            } else {
                // Write the record as is if no match
                tempStream << line << endl;
            }
        }

        originalFile.close();
        tempStream.close();

        // Replace the original file with the temp file
        remove(appointmentFilePath.c_str());
        rename("temp_appointments.txt", appointmentFilePath.c_str());

        rebuildIndices();  // Rebuild indices after the update
        cout << "Appointment updated successfully!\n";
    }



    // Display Menu
    void displayMenu() {
        cout << "\nHealthcare Management System\n";
        cout << "1. Add New Doctor\n";
        cout << "2. Delete Doctor\n";
        cout << "3. Update Doctor Name\n";
        cout << "4. Search Doctor by ID\n";
        cout << "5. Add New Appointment\n";
        cout << "6. Search Appointment by ID\n";
        cout<<  "7. Delete Appointment\n";
        cout<<"8. Update Appointment\n";
        cout<<"9. Write query\n";
        cout << "0. Exit\n";
        cout << "Enter your choice: ";
    }

    // Run Menu
    void runMenu() {
        int choice;
        string doctorId;
        string appointmentId;
        do {
            displayMenu();
            if (!(cin >> choice)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input. Please enter a number.\n";
                continue;
            }

            switch (choice) {
                case 1:
                    add_doctor();
                    break;
                case 2:
                    deleteDoctor(); // Implement deleteDoctor function
                    break;
                case 3:
                    updateDoctor(); // Implement updateDoctorName function
                    break;
                case 4:
                    cout << "Enter Doctor ID to search: ";
                    cin >> doctorId;
                    searchDoctor(doctorId);
                    break;
                case 5:
                    addAppointment();
                    break;
                case 6:

                    cout << "Enter Appointment ID to search: ";
                    cin >> appointmentId;
                    searchAppointment(appointmentId);
                    break;
                case 7:
                    cout<<"Enter the Doctor Id associated with this appointment";
                    cin>>doctorId;
                    deleteRelatedAppointments(doctorId);
                    rebuildIndices();
                    break;
                case 8:
                    updateAppointment();
                    break;
                case 9:
                    execute_query();
                    break;
                case 0:
                    cout << "Exiting the system. Goodbye!\n";
                    break;
                default:
                    cout << "Invalid choice. Please try again.\n";
                    break;
            }
        } while (choice != 0);
    }
    void execute_query() {
        string query;
        cout << "please enter query in this forumla: Select column_name from table where cond\n";
        cin.ignore();
        getline(cin, query);
        stringstream ss(query);
        string word;

        // Parse the query into components
        string command, table, column_name, cond, value;
        ss >> command; // Select
        ss >> column_name; // e.g., "all"
        ss >> word; // from
        ss >> table; // Table name, e.g., "Doctors"
        ss >> word; // where
        ss >> cond; //eg Doctor ID
        ss >> word; // =
        ss >> value; // The value of Doctor ID, e.g., 'xxx'


        // Handle the query based on the table and column requested
        if (command != "Select") {
            cout << "Invalid query format, only select is provided\n";
            return;
        }
        if (column_name == "all") {
            if (table == "Doctors" && cond == "DoctorID") {
//                auto it = binarySearchPrimaryIdx(value);
                searchDoctor(value);
            } else if (table == "Appointments" && cond == "AppointmentID") {
//                auto it = binarySearchPrimaryIdx(appointmentPrimaryIndex, value);
//                printAppointmentInfo(it->first);
                searchAppointment(value);
            }
//            else if(table=="Appointments"&& cond=="DoctorID"){
//                searchAppointment(value);
//            }
//        }
//        else if(column_name=="DoctorName") {
//            searchDoctorByName(value);
//        }

        }
    }
    int binarySearch(const vector<IndexEntry>& index, const string& key) {
        int low = 0, high = index.size() - 1;
        while (low <= high) {
            int mid = low + (high - low) / 2;
            if (index[mid].key == key) return index[mid].position;
            else if (index[mid].key < key) low = mid + 1;
            else high = mid - 1;
        }
        return -1;
    }


};

int main() {

    Healthcare system;
    system.runMenu();
    return 0;
}
