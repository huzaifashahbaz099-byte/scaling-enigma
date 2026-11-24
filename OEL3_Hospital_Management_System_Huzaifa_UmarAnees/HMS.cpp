// =============================================================================
//  Hospital Appointment and Patient Management System          (OEL-3, DSA)
//  Group members : Huzaifa, Umar Anees
//
//  What the program does
//    * Keeps patient records       -> singly linked list
//    * Keeps appointment history   -> array of structures (with a status)
//    * Serves appointments in order of arrival -> FIFO queue (linked nodes)
//    * Searching : linear search (by name) and binary search (by ID)
//    * Sorting   : insertion sort (patients) and selection sort (appointments)
//    * Saves everything in patients.txt and appointments.txt and reloads it
//      when the program is started again.
//
//  The file is arranged from the lowest level to the highest level:
//    1. constants and structures      5. patient list helpers
//    2. the appointment queue         6. file handling
//    3. global data                   7. patient module
//    4. input and validation helpers  8. searching and sorting
//                                     9. appointment module
//                                    10. menu and main()
// =============================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <cstdlib>
#include <cctype>
using namespace std;

// =============================================================================
// 1. CONSTANTS AND STRUCTURES
// =============================================================================
const int MAX_APPT = 500;                  // size of the appointment array
const int MIN_YEAR = 2025;                 // earliest year accepted for a date
const int MAX_YEAR = 2100;                 // latest year accepted for a date
const int CLINIC_OPEN  = 9 * 60;           // 09:00 in minutes after midnight
const int CLINIC_CLOSE = 17 * 60;          // 17:00 in minutes after midnight

const string PATIENT_FILE = "patients.txt";
const string APPT_FILE    = "appointments.txt";

const string SCHEDULED = "Scheduled";      // the three appointment statuses
const string COMPLETED = "Completed";
const string CANCELLED = "Cancelled";

// One patient = one node of the singly linked list.
struct Patient {
    int id;
    string name;
    int age;
    string gender;                         // "M" or "F"
    string disease;
    Patient* next;                         // next patient in the list
};

// One appointment record. All records live in the array appts[].
struct Appointment {
    int appId;
    int patientId;
    string doctor;
    string date;                           // DD-MM-YYYY
    string time;                           // HH:MM
    string status;                         // Scheduled / Completed / Cancelled
};

// One node of the waiting queue. It stores the ADDRESS of a record in appts[]
// (not a copy), so the queue and the array always agree with each other.
struct QNode {
    Appointment* data;
    QNode* next;
};

// =============================================================================
// 2. THE APPOINTMENT QUEUE  (First In, First Out)
// =============================================================================
class AppointmentQueue {
private:
    QNode* front;                          // next appointment to be served
    QNode* rear;                           // last appointment that was booked
    int count;                             // how many appointments are waiting

public:
    AppointmentQueue() : front(NULL), rear(NULL), count(0) {}

    bool isEmpty() const { return front == NULL; }
    int size() const { return count; }

    // Add an appointment at the rear of the line.                    O(1)
    void enqueue(Appointment* a) {
        QNode* node = new QNode;
        node->data = a;
        node->next = NULL;
        if (rear == NULL) {                // queue was empty
            front = node;
            rear = node;
        } else {
            rear->next = node;
            rear = node;
        }
        count++;
    }

    // Remove and return the appointment at the front of the line.   O(1)
    Appointment* dequeue() {
        if (front == NULL) return NULL;
        QNode* old = front;
        Appointment* a = old->data;
        front = front->next;
        if (front == NULL) rear = NULL;    // queue became empty
        delete old;
        count--;
        return a;
    }

    // Remove one appointment from ANY position (used when it is cancelled
    // or when its patient is deleted).                                O(n)
    bool removeById(int appId) {
        QNode* cur = front;
        QNode* prev = NULL;
        while (cur != NULL) {
            if (cur->data->appId == appId) {
                if (prev == NULL) front = cur->next;   // removing the first node
                else prev->next = cur->next;
                if (cur == rear) rear = prev;          // removing the last node
                delete cur;
                count--;
                return true;
            }
            prev = cur;
            cur = cur->next;
        }
        return false;
    }

    // Print the line from front to rear.
    void display() const;                  // written in section 9
};

// =============================================================================
// 3. GLOBAL DATA
// =============================================================================
Patient* head = NULL;                      // first node of the patient list
Appointment appts[MAX_APPT];               // every appointment ever booked
int apptCount = 0;                         // number of used slots in appts[]
int nextAppId = 1;                         // ID given to the next appointment
AppointmentQueue waitingQueue;             // appointments still Scheduled

// =============================================================================
// 4. INPUT AND VALIDATION HELPERS
// =============================================================================

// Remove spaces, tabs and line ends from both ends of a text.
string trim(const string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

// Show a prompt and read one whole line (so names with spaces work).
// Returns false when the input has ended.
bool readLine(const string& prompt, string& out) {
    cout << prompt;
    if (!getline(cin, out)) return false;
    out = trim(out);
    return true;
}

// Convert text to an integer, but only if it contains digits only.
bool toInt(const string& s, int& value) {
    if (s.empty() || s.size() > 9) return false;
    for (size_t i = 0; i < s.size(); i++) {
        if (!isdigit((unsigned char)s[i])) return false;
    }
    value = atoi(s.c_str());
    return true;
}

// '|' separates the fields in the data files, so it is not allowed in text.
bool hasBar(const string& s) {
    return s.find('|') != string::npos;
}

string toLower(string s) {
    for (size_t i = 0; i < s.size(); i++) {
        s[i] = (char)tolower((unsigned char)s[i]);
    }
    return s;
}

void printError(const string& message) {
    cout << "Error: " << message << endl;
}

bool isLeap(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Check a DD-MM-YYYY date and split it into day, month and year.
bool parseDate(const string& d, int& dd, int& mm, int& yy) {
    if (d.size() != 10 || d[2] != '-' || d[5] != '-') return false;
    if (!toInt(d.substr(0, 2), dd)) return false;
    if (!toInt(d.substr(3, 2), mm)) return false;
    if (!toInt(d.substr(6, 4), yy)) return false;
    if (yy < MIN_YEAR || yy > MAX_YEAR) return false;
    if (mm < 1 || mm > 12 || dd < 1) return false;

    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maxDay = daysInMonth[mm - 1];
    if (mm == 2 && isLeap(yy)) maxDay = 29;
    return dd <= maxDay;
}

// Check an HH:MM time that lies inside the clinic hours (09:00 - 17:00).
bool parseTime(const string& t, int& hh, int& mn) {
    if (t.size() != 5 || t[2] != ':') return false;
    if (!toInt(t.substr(0, 2), hh) || !toInt(t.substr(3, 2), mn)) return false;
    if (hh > 23 || mn > 59) return false;
    int minutes = hh * 60 + mn;
    return minutes >= CLINIC_OPEN && minutes <= CLINIC_CLOSE;
}

// Sorting keys: a date as a number YYYYMMDD and a time as minutes.
// (Numbers compare correctly, DD-MM-YYYY text does not.)
int dateKey(const string& d) {
    int dd, mm, yy;
    parseDate(d, dd, mm, yy);
    return yy * 10000 + mm * 100 + dd;
}

int timeKey(const string& t) {
    int hh, mn;
    parseTime(t, hh, mn);
    return hh * 60 + mn;
}

// =============================================================================
// 5. PATIENT LIST HELPERS
// =============================================================================

// Linear search in the linked list. Returns NULL if the ID does not exist.
Patient* findPatient(int id) {
    for (Patient* p = head; p != NULL; p = p->next) {
        if (p->id == id) return p;
    }
    return NULL;
}

int patientCount() {
    int n = 0;
    for (Patient* p = head; p != NULL; p = p->next) n++;
    return n;
}

// Add a node at the END of the list (keeps registration order).
void appendPatient(Patient* node) {
    node->next = NULL;
    if (head == NULL) {
        head = node;
        return;
    }
    Patient* last = head;
    while (last->next != NULL) last = last->next;
    last->next = node;
}

void printPatientHeader() {
    cout << left << setw(8) << "ID" << setw(20) << "Name" << setw(6) << "Age"
         << setw(8) << "Gender" << "Disease" << endl;
    cout << "------------------------------------------------------------" << endl;
}

void printPatient(const Patient* p) {
    cout << left << setw(8) << p->id << setw(20) << p->name << setw(6) << p->age
         << setw(8) << p->gender << p->disease << endl;
}

// =============================================================================
// 6. FILE HANDLING
//    patients.txt     : id|name|age|gender|disease
//    appointments.txt : appId|patientId|doctor|date|time|status
// =============================================================================
void savePatients() {
    ofstream file(PATIENT_FILE.c_str());
    for (Patient* p = head; p != NULL; p = p->next) {
        file << p->id << "|" << p->name << "|" << p->age << "|"
             << p->gender << "|" << p->disease << "\n";
    }
}

void saveAppointments() {
    ofstream file(APPT_FILE.c_str());
    for (int i = 0; i < apptCount; i++) {
        file << appts[i].appId << "|" << appts[i].patientId << "|"
             << appts[i].doctor << "|" << appts[i].date << "|"
             << appts[i].time << "|" << appts[i].status << "\n";
    }
}

// Read both files at start-up and rebuild the list, the array and the queue.
void loadData() {
    int loadedPatients = 0, loadedAppointments = 0;
    string line;

    // ---- patients -> linked list
    ifstream pf(PATIENT_FILE.c_str());
    while (getline(pf, line)) {
        if (trim(line).empty()) continue;
        stringstream ss(line);
        string id, name, age, gender, disease;
        getline(ss, id, '|');
        getline(ss, name, '|');
        getline(ss, age, '|');
        getline(ss, gender, '|');
        getline(ss, disease, '|');

        Patient* node = new Patient;
        if (!toInt(id, node->id) || !toInt(age, node->age)) {   // bad line
            delete node;
            continue;
        }
        node->name = name;
        node->gender = gender;
        node->disease = disease;
        appendPatient(node);
        loadedPatients++;
    }

    // ---- appointments -> array, and Scheduled ones -> queue (file order)
    ifstream af(APPT_FILE.c_str());
    while (apptCount < MAX_APPT && getline(af, line)) {
        if (trim(line).empty()) continue;
        stringstream ss(line);
        string appId, patientId, doctor, date, time, status;
        getline(ss, appId, '|');
        getline(ss, patientId, '|');
        getline(ss, doctor, '|');
        getline(ss, date, '|');
        getline(ss, time, '|');
        getline(ss, status, '|');

        Appointment a;
        if (!toInt(appId, a.appId) || !toInt(patientId, a.patientId)) continue;
        a.doctor = doctor;
        a.date = date;
        a.time = time;
        a.status = status;

        appts[apptCount] = a;
        if (a.status == SCHEDULED) waitingQueue.enqueue(&appts[apptCount]);
        if (a.appId >= nextAppId) nextAppId = a.appId + 1;   // IDs never repeat
        apptCount++;
        loadedAppointments++;
    }

    cout << "Data loaded: " << loadedPatients << " patient(s), "
         << loadedAppointments << " appointment(s)." << endl;
}

// =============================================================================
// 7. PATIENT MODULE  (register, update, delete)
// =============================================================================

// Cancel a Scheduled appointment: change its status AND take it out of the
// queue. Used by cancelAppointment() and by deletePatient().
void cancelScheduled(Appointment& a) {
    a.status = CANCELLED;
    waitingQueue.removeById(a.appId);
}

void registerPatient() {
    string text, name, gender, disease;
    int id, age;

    if (!readLine("Enter Patient ID (1-99999): ", text)) return;
    if (!toInt(text, id) || id < 1 || id > 99999) {
        printError("invalid Patient ID.");
        return;
    }
    if (findPatient(id) != NULL) {
        cout << "Error: Patient ID " << id << " already exists." << endl;
        return;
    }

    if (!readLine("Enter Name: ", name)) return;
    if (name.empty() || hasBar(name)) {
        printError("invalid name.");
        return;
    }

    if (!readLine("Enter Age (1-120): ", text)) return;
    if (!toInt(text, age) || age < 1 || age > 120) {
        printError("invalid age.");
        return;
    }

    if (!readLine("Enter Gender (M/F): ", gender)) return;
    gender = toLower(gender);
    if (gender == "m") gender = "M";
    else if (gender == "f") gender = "F";
    else {
        printError("gender must be M or F.");
        return;
    }

    if (!readLine("Enter Disease: ", disease)) return;
    if (disease.empty() || hasBar(disease)) {
        printError("invalid disease.");
        return;
    }

    // every check passed, so the new node can be added safely
    Patient* node = new Patient;
    node->id = id;
    node->name = name;
    node->age = age;
    node->gender = gender;
    node->disease = disease;
    appendPatient(node);
    savePatients();
    cout << "Patient registered successfully." << endl;
}

void updatePatient() {
    string text;
    int id;
    if (!readLine("Enter Patient ID to update: ", text)) return;
    if (!toInt(text, id)) {
        printError("invalid Patient ID.");
        return;
    }
    Patient* p = findPatient(id);
    if (p == NULL) {
        printError("patient not found.");
        return;
    }

    cout << "Current record: " << p->name << ", " << p->age << ", "
         << p->gender << ", " << p->disease << endl;
    cout << "(press Enter to keep the old value)" << endl;

    string newName, newAge, newDisease;
    if (!readLine("New Name: ", newName)) return;
    if (!readLine("New Age: ", newAge)) return;
    if (!readLine("New Disease: ", newDisease)) return;

    // validate everything first, so the update is all-or-nothing
    int age = 0;
    if (!newAge.empty() && (!toInt(newAge, age) || age < 1 || age > 120)) {
        printError("invalid age. Nothing updated.");
        return;
    }
    if (hasBar(newName) || hasBar(newDisease)) {
        printError("invalid input. Nothing updated.");
        return;
    }

    if (!newName.empty()) p->name = newName;
    if (!newAge.empty()) p->age = age;
    if (!newDisease.empty()) p->disease = newDisease;
    savePatients();
    cout << "Patient record updated." << endl;
}

void deletePatient() {
    string text;
    int id;
    if (!readLine("Enter Patient ID to delete: ", text)) return;
    if (!toInt(text, id)) {
        printError("invalid Patient ID.");
        return;
    }

    // find the node and the node before it
    Patient* cur = head;
    Patient* prev = NULL;
    while (cur != NULL && cur->id != id) {
        prev = cur;
        cur = cur->next;
    }
    if (cur == NULL) {
        printError("patient not found.");
        return;
    }

    // unlink the node, then free it
    if (prev == NULL) head = cur->next;        // deleting the first patient
    else prev->next = cur->next;
    delete cur;

    // a deleted patient must never be served: cancel the Scheduled ones
    int cancelled = 0;
    for (int i = 0; i < apptCount; i++) {
        if (appts[i].patientId == id && appts[i].status == SCHEDULED) {
            cancelScheduled(appts[i]);
            cancelled++;
        }
    }
    savePatients();
    saveAppointments();
    cout << "Patient deleted. " << cancelled
         << " scheduled appointment(s) cancelled." << endl;
}

// =============================================================================
// 8. SEARCHING AND SORTING
// =============================================================================

// A linked list has no index, so the patients are copied (as addresses)
// into a temporary array. The caller must free it with delete[].
Patient** listToArray(int& n) {
    n = patientCount();
    Patient** arr = new Patient*[n > 0 ? n : 1];
    int i = 0;
    for (Patient* p = head; p != NULL; p = p->next) arr[i++] = p;
    return arr;
}

// Insertion sort by patient ID.
void insertionSortById(Patient** a, int n) {
    for (int i = 1; i < n; i++) {
        Patient* key = a[i];
        int j = i - 1;
        while (j >= 0 && a[j]->id > key->id) {   // shift bigger IDs to the right
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
}

// Insertion sort by patient name (case-insensitive).
void insertionSortByName(Patient** a, int n) {
    for (int i = 1; i < n; i++) {
        Patient* key = a[i];
        int j = i - 1;
        while (j >= 0 && toLower(a[j]->name) > toLower(key->name)) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
}

// Binary search in an array that is already sorted by ID.
// Returns the position of the ID, or -1 if it does not exist.
int binarySearchById(Patient** a, int n, int id) {
    int lo = 0, hi = n - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (a[mid]->id == id) return mid;
        if (a[mid]->id < id) lo = mid + 1;       // look in the right half
        else hi = mid - 1;                       // look in the left half
    }
    return -1;
}

void searchPatient() {
    string text;
    cout << "1. Search by ID (binary search)" << endl;
    cout << "2. Search by Name (linear search)" << endl;
    if (!readLine("Choice: ", text)) return;

    if (text == "1") {
        int id;
        if (!readLine("Enter Patient ID: ", text)) return;
        if (!toInt(text, id)) {
            printError("invalid Patient ID.");
            return;
        }
        int n;
        Patient** arr = listToArray(n);          // binary search needs a sorted array
        insertionSortById(arr, n);
        int pos = binarySearchById(arr, n, id);
        if (pos < 0) {
            cout << "Patient not found." << endl;
        } else {
            printPatientHeader();
            printPatient(arr[pos]);
        }
        delete[] arr;
    } else if (text == "2") {
        string key;
        if (!readLine("Enter name (or part of name): ", key)) return;
        if (key.empty()) {
            printError("empty search text.");
            return;
        }
        key = toLower(key);
        bool found = false;
        for (Patient* p = head; p != NULL; p = p->next) {   // linear search
            if (toLower(p->name).find(key) != string::npos) {
                if (!found) printPatientHeader();
                printPatient(p);
                found = true;
            }
        }
        if (!found) cout << "Patient not found." << endl;
    } else {
        printError("invalid choice.");
    }
}

void displayPatients() {
    if (head == NULL) {
        cout << "No patient records available." << endl;
        return;
    }
    string text;
    cout << "Sort by: 1. ID   2. Name" << endl;
    if (!readLine("Choice: ", text)) return;
    if (text != "1" && text != "2") {
        printError("invalid choice.");
        return;
    }

    int n;
    Patient** arr = listToArray(n);              // sort the addresses only
    if (text == "1") insertionSortById(arr, n);
    else insertionSortByName(arr, n);

    printPatientHeader();
    for (int i = 0; i < n; i++) printPatient(arr[i]);
    cout << "Total patients: " << n << endl;
    delete[] arr;
}

// =============================================================================
// 9. APPOINTMENT MODULE
// =============================================================================

// pos > 0 prints the queue position column too (used by the waiting queue).
void printAppointmentHeader(bool withPosition) {
    cout << left;
    if (withPosition) cout << setw(6) << "Pos";
    cout << setw(8) << "AppID" << setw(10) << "PatientID" << setw(18) << "Doctor"
         << setw(13) << "Date" << setw(8) << "Time" << "Status" << endl;
    cout << "------------------------------------------------------------------" << endl;
}

void printAppointment(const Appointment* a, int pos) {
    cout << left;
    if (pos > 0) cout << setw(6) << pos;
    cout << setw(8) << a->appId << setw(10) << a->patientId << setw(18) << a->doctor
         << setw(13) << a->date << setw(8) << a->time << a->status << endl;
}

void AppointmentQueue::display() const {
    printAppointmentHeader(true);
    int pos = 1;
    for (QNode* cur = front; cur != NULL; cur = cur->next) {
        printAppointment(cur->data, pos++);
    }
}

void bookAppointment() {
    string text, doctor, date, time;
    int patientId;

    if (!readLine("Enter Patient ID: ", text)) return;
    if (!toInt(text, patientId)) {
        printError("invalid Patient ID.");
        return;
    }
    if (findPatient(patientId) == NULL) {
        printError("patient not found. Register the patient first.");
        return;
    }

    if (!readLine("Enter Doctor name: ", doctor)) return;
    if (doctor.empty() || hasBar(doctor)) {
        printError("invalid doctor name.");
        return;
    }

    int dd, mm, yy;
    if (!readLine("Enter Date (DD-MM-YYYY): ", date)) return;
    if (!parseDate(date, dd, mm, yy)) {
        printError("invalid date.");
        return;
    }

    int hh, mn;
    if (!readLine("Enter Time (HH:MM, 09:00-17:00): ", time)) return;
    if (!parseTime(time, hh, mn)) {
        printError("invalid time.");
        return;
    }

    if (apptCount >= MAX_APPT) {
        printError("appointment list is full.");
        return;
    }

    // the same doctor cannot have two Scheduled appointments at one date/time
    for (int i = 0; i < apptCount; i++) {
        if (appts[i].status == SCHEDULED &&
            toLower(appts[i].doctor) == toLower(doctor) &&
            appts[i].date == date && appts[i].time == time) {
            printError("this doctor already has an appointment at that date and time.");
            return;
        }
    }

    // store the record in the array, and its address at the rear of the queue
    Appointment& a = appts[apptCount];
    a.appId = nextAppId++;
    a.patientId = patientId;
    a.doctor = doctor;
    a.date = date;
    a.time = time;
    a.status = SCHEDULED;
    waitingQueue.enqueue(&a);
    apptCount++;
    saveAppointments();

    cout << "Appointment booked. Appointment ID = " << a.appId
         << " (queue position " << waitingQueue.size() << ")." << endl;
}

void processNext() {
    if (waitingQueue.isEmpty()) {
        cout << "No appointments in the queue." << endl;
        return;
    }
    Appointment* a = waitingQueue.dequeue();     // the one who arrived first
    a->status = COMPLETED;
    saveAppointments();
    cout << "Appointment " << a->appId << " (Patient " << a->patientId << ", "
         << a->doctor << ") marked as Completed." << endl;
}

void cancelAppointment() {
    string text;
    int id;
    if (!readLine("Enter Appointment ID to cancel: ", text)) return;
    if (!toInt(text, id)) {
        printError("invalid Appointment ID.");
        return;
    }

    for (int i = 0; i < apptCount; i++) {        // linear search by appointment ID
        if (appts[i].appId == id) {
            if (appts[i].status != SCHEDULED) {
                cout << "Error: appointment is already " << appts[i].status
                     << " and cannot be cancelled." << endl;
                return;
            }
            cancelScheduled(appts[i]);
            saveAppointments();
            cout << "Appointment " << id << " cancelled." << endl;
            return;
        }
    }
    cout << "Error: appointment " << id << " does not exist." << endl;
}

void viewQueue() {
    if (waitingQueue.isEmpty()) {
        cout << "No appointments in the queue." << endl;
        return;
    }
    waitingQueue.display();
}

// Selection sort by date, then by time. Works on an array of addresses, so
// the appts[] array and the queue are not disturbed.
void selectionSortAppointments(Appointment** a, int n) {
    for (int i = 0; i < n - 1; i++) {
        int smallest = i;
        for (int j = i + 1; j < n; j++) {
            int dateJ = dateKey(a[j]->date), dateS = dateKey(a[smallest]->date);
            if (dateJ < dateS ||
                (dateJ == dateS && timeKey(a[j]->time) < timeKey(a[smallest]->time))) {
                smallest = j;
            }
        }
        Appointment* temp = a[i];
        a[i] = a[smallest];
        a[smallest] = temp;
    }
}

void viewAllAppointments() {
    if (apptCount == 0) {
        cout << "No appointments available." << endl;
        return;
    }
    Appointment** arr = new Appointment*[apptCount];
    for (int i = 0; i < apptCount; i++) arr[i] = &appts[i];
    selectionSortAppointments(arr, apptCount);

    printAppointmentHeader(false);
    for (int i = 0; i < apptCount; i++) printAppointment(arr[i], 0);
    delete[] arr;
}

// =============================================================================
// 10. MENU AND main()
// =============================================================================
void showMenu() {
    cout << endl;
    cout << "===== HOSPITAL APPOINTMENT AND PATIENT MANAGEMENT SYSTEM =====" << endl;
    cout << " 1. Register patient" << endl;
    cout << " 2. Update patient" << endl;
    cout << " 3. Delete patient" << endl;
    cout << " 4. Search patient" << endl;
    cout << " 5. Display patients (sorted)" << endl;
    cout << " 6. Book appointment" << endl;
    cout << " 7. Process next appointment" << endl;
    cout << " 8. Cancel appointment" << endl;
    cout << " 9. View waiting queue" << endl;
    cout << "10. View all appointments (sorted by date and time)" << endl;
    cout << " 0. Save and exit" << endl;
}

int main() {
    loadData();
    string text;
    while (true) {
        showMenu();
        if (!readLine("Enter choice: ", text)) break;

        int choice = -1;
        if (!toInt(text, choice)) choice = -1;   // letters etc. -> invalid

        if (choice == 0) break;
        switch (choice) {
            case 1:  registerPatient();      break;
            case 2:  updatePatient();        break;
            case 3:  deletePatient();        break;
            case 4:  searchPatient();        break;
            case 5:  displayPatients();      break;
            case 6:  bookAppointment();      break;
            case 7:  processNext();          break;
            case 8:  cancelAppointment();    break;
            case 9:  viewQueue();            break;
            case 10: viewAllAppointments();  break;
            default: cout << "Invalid choice. Please try again." << endl;
        }
    }
    savePatients();
    saveAppointments();
    cout << "Data saved. Goodbye!" << endl;
    return 0;
}
