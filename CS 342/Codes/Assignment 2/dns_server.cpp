#include "utils.h"
using namespace std;

unsigned char buf[LEN];
int readlen;

// DNS Cache declaration
unordered_map<string, vector<pair<string, string>>> dns_cache;
vector<struct RES_RECORD> answers, auth, addit;

// Handle cache hit
void handle_cache_hit(string host_name)
{
    cout << "It's a cache hit. Fetching from the cache..." << endl;

    // Print the cache
    int ans_size = stoi(dns_cache[host_name][dns_cache[host_name].size() - 3].second);
    int auth_size = stoi(dns_cache[host_name][dns_cache[host_name].size() - 2].second);
    int addit_size = stoi(dns_cache[host_name][dns_cache[host_name].size() - 1].second);

    cout << "\nNumber of Answer Records : " << ans_size << endl;
    for (int i = 0; i < ans_size; i++)
    {
        if (isdigit(dns_cache[host_name][i].second[0]))
        {
            cout << "Name : " << dns_cache[host_name][i].first.c_str();
            cout << ", IPv4 address : " << dns_cache[host_name][i].second.c_str() << endl;
        }
        else
        {
            cout << "Name : " << dns_cache[host_name][i].first.c_str();
            cout << ", Alias name : " << dns_cache[host_name][i].second.c_str() << endl;
        }
    }

    cout << "\nAuthoritive Records : " << auth_size << endl;
    for (int i = ans_size; i < ans_size + auth_size; i++)
    {
        cout << "Name : " << dns_cache[host_name][i].first.c_str();
        cout << ", Name Server : " << dns_cache[host_name][i].second.c_str() << endl;
    }

    cout << "\nAdditional Records : " << addit_size << endl;
    for (int i = ans_size + auth_size; i < ans_size + auth_size + addit_size; i++)
    {
        cout << "Name : " << dns_cache[host_name][i].first.c_str();
        cout << ", IPv4 address : " << dns_cache[host_name][i].second.c_str() << endl;
    }
    cout << endl;
}

// Function  convert www.google.com to 3www6google3com
void URL_Formatter(unsigned char *dns, unsigned char *host)
{
    stringstream ss;
    string s(reinterpret_cast<char *>(host));
    ss << s;
    string temp = "";
    string str;

    while (getline(ss, str, '.'))
    {
        temp += str.size();
        temp += str;
    }

    strcpy(reinterpret_cast<char *>(dns), temp.c_str());
}

// To Parse the host-name from the DNS packet
u_char *Parse_Name(unsigned char *ptr, unsigned char *buffer, int *bytesRead)
{
    /*
    In DNS packets, labels can be either regular labels (indicating a sequence of characters) or pointers to previous occurrences of labels in the packet. Pointers are used for compression to save space in DNS packets. The value 192 corresponds to binary * 1000000, and this is used as the first two bits in a DNS label to indicate that it's a pointer.
    */

    unsigned char *name;
    unsigned int p = 0, offset;
    bool jumped = 0;
    int i, j;

    *bytesRead = 1;
    name = (unsigned char *)malloc(256);

    name[0] = '\0';

    // read the names in 3www6google3com format
    while (*ptr != 0)
    {
        if (*ptr >= 192)
        {
            offset = (*ptr) * (1 << 8) + *(ptr + 1) - 49152; // 49152 = 11000000 00000000
            ptr = buffer + offset - 1;
            jumped = 1; // Go to an offset
        }
        else
        {
            name[p++] = *ptr;
        }

        ptr = ptr + 1;

        if (jumped == 0)
        {
            *bytesRead = *bytesRead + 1; // if we havent jumped to another location then we can count up
        }
    }

    name[p] = '\0'; // string complete
    if (jumped == 1)
    {
        *bytesRead = *bytesRead + 1; // number of steps we actually moved forward in the packet
    }

    // now convert 3www6google3com0 to www.google.com
    for (i = 0; i < (int)strlen((const char *)name); i++)
    {
        p = name[i];
        for (j = 0; j < (int)p; j++)
        {
            name[i] = name[i + 1];
            i = i + 1;
        }
        name[i] = '.';
    }
    name[i - 1] = '\0'; // Remove the last dot
    return name;
}

// Read the necessary data from packet
void Read_Answers(int ans_cnt, unsigned char *ptr, string host_name)
{
    for (int i = 0; i < ans_cnt; i++)
    {
        struct RES_RECORD temp;
        temp.name = Parse_Name(ptr, buf, &readlen);
        ptr = ptr + readlen;

        temp.resource = (struct R_DATA *)(ptr);
        ptr = ptr + sizeof(struct R_DATA);

        if (ntohs(temp.resource->type) == T_A) // If its Type A record - IPv4
        {
            temp.rdata = (unsigned char *)malloc(ntohs(temp.resource->data_len));

            for (int j = 0; j < ntohs(temp.resource->data_len); j++)
            {
                temp.rdata[j] = ptr[j];
            }

            temp.rdata[ntohs(temp.resource->data_len)] = '\0';

            ptr = ptr + ntohs(temp.resource->data_len);

            answers.push_back(temp);
        }
        else
        {
            temp.rdata = Parse_Name(ptr, buf, &readlen);
            ptr = ptr + readlen;
        }
    }
}

void Read_Authorities(int auth_cnt, unsigned char *ptr, string host_name)
{
    for (int i = 0; i < auth_cnt; i++)
    {
        struct RES_RECORD temp;
        temp.name = Parse_Name(ptr, buf, &readlen);
        string name(reinterpret_cast<char *>(temp.name));
        if (name == "")
        {
            temp.name = (unsigned char *)host_name.c_str();
        }
        ptr += readlen;

        temp.resource = (struct R_DATA *)(ptr);
        ptr += sizeof(struct R_DATA);

        temp.rdata = Parse_Name(ptr, buf, &readlen);
        ptr += readlen;

        auth.push_back(temp);
    }
}

void Read_Additional(int add_cnt, unsigned char *ptr, string host_name)
{
    for (int i = 0; i < add_cnt; i++)
    {
        struct RES_RECORD temp;
        temp.name = Parse_Name(ptr, buf, &readlen);
        ptr += readlen;

        temp.resource = (struct R_DATA *)(ptr);
        ptr += sizeof(struct R_DATA);

        if (ntohs(temp.resource->type) == T_A)
        {
            temp.rdata = (unsigned char *)malloc(ntohs(temp.resource->data_len));
            for (int j = 0; j < ntohs(temp.resource->data_len); j++)
                temp.rdata[j] = ptr[j];

            temp.rdata[ntohs(temp.resource->data_len)] = '\0';
            ptr += ntohs(temp.resource->data_len);

            addit.push_back(temp);
        }
        else
        {
            temp.rdata = Parse_Name(ptr, buf, &readlen);
            ptr += readlen;
        }
    }
}

// Print the data
void Print_Packets(struct sockaddr_in &a, string host_name)
{
    // print answers
    cout << "\nNumber of Answer Records : " << (int)answers.size() << endl;
    for (int i = 0; i < (int)answers.size(); i++)
    {
        struct RES_RECORD temp = answers[i];

        if (ntohs(temp.resource->type) == T_A) // IPv4 address
        {
            long *p;
            p = (long *)temp.rdata;
            a.sin_addr.s_addr = (*p);

            // Add to cache
            dns_cache[host_name].push_back(make_pair(string(reinterpret_cast<char *>(temp.name)), inet_ntoa(a.sin_addr)));
            cout << "Name: " << temp.name;
            cout << ", IPv4 address : " << inet_ntoa(a.sin_addr) << endl;
        }

        if (ntohs(temp.resource->type) == T_CNAME)
        {
            // Canonical name for an alias
            dns_cache[host_name].push_back(make_pair(string(reinterpret_cast<char *>(temp.name)), string(reinterpret_cast<char *>(temp.rdata))));

            cout << "Name: " << temp.name;
            cout << ", Alias Name : " << temp.rdata << endl;
        }
    }

    // print authorities

    cout << "\nNumber of Authoritative Records : " << (int)auth.size() << endl;
    for (int i = 0; i < (int)auth.size(); i++)
    {
        struct RES_RECORD temp = auth[i];

        dns_cache[host_name].push_back(make_pair(string(reinterpret_cast<char *>(temp.name)), string(reinterpret_cast<char *>(temp.rdata))));

        cout << "Name: " << temp.name;
        cout << ", Name Server : " << temp.rdata << endl;
    }

    // print additional resource records

    cout << "\nNumber of Additional Records : " << (int)auth.size() << endl;
    for (int i = 0; i < (int)addit.size(); i++)
    {
        struct RES_RECORD temp = addit[i];

        long *p;
        p = (long *)temp.rdata;
        a.sin_addr.s_addr = (*p);

        // Add to cache
        dns_cache[host_name].push_back(make_pair(string(reinterpret_cast<char *>(temp.name)), inet_ntoa(a.sin_addr)));

        cout << "Name: " << temp.name;
        cout << ", IPv4 address : " << inet_ntoa(a.sin_addr) << endl;
    }

    cout << endl;
}

// Send DNS Packet
void DNS_Resolver(unsigned char *host, int query_type)
{
    cout << "\nLooking in cache..." << endl;
    string host_name(reinterpret_cast<char *>(host));

    if (dns_cache.find(host_name) != dns_cache.end())
    {
        handle_cache_hit(host_name);
        return;
    }

    cout << "Oops, it's a cache miss. Preparing DNS query packet..." << endl;

    unsigned char *qname, *ptr;
    int i, j;

    struct sockaddr_in temp_addr;
    struct sockaddr_in dest;

    struct DNS_HEADER *dns = NULL;
    struct QUESTION *qinfo = NULL;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // UDP packet for DNS queries

    dest.sin_family = AF_INET;
    dest.sin_port = htons(dns_PORT);
    dest.sin_addr.s_addr = inet_addr(dns_IP); // dns servers

    // Set the DNS structure to standard queries
    dns = (struct DNS_HEADER *)&buf;

    dns->id = (unsigned short)htons(getpid()); // making the query id to be PID of the process
    dns->qr = 0;                               // This is a query
    dns->opcode = 0;                           // This is a standard query
    dns->aa = 0;                               // Not Authoritative
    dns->tc = 0;                               // This message is not truncated
    dns->rd = 1;                               // Recursion Desired
    dns->ra = 0;                               // Recursion not available from client side
    dns->z = 0;
    dns->ad = 0;
    dns->cd = 0;
    dns->rcode = 0;
    dns->q_count = htons(1); // Number of Questions
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;

    // point to the query portion
    qname = (unsigned char *)&buf[sizeof(struct DNS_HEADER)];

    URL_Formatter(qname, host);

    // Question Section
    qinfo = (struct QUESTION *)&buf[sizeof(struct DNS_HEADER) + (strlen((const char *)qname) + 1)];
    qinfo->qtype = htons(query_type); // type of the query , A , MX , CNAME , NS etc
    qinfo->qclass = htons(1);         // For internet

    cout << "Sending the DNS Query packet to " << dns_IP << endl;
    int QuerySize = sizeof(struct DNS_HEADER) + (strlen((const char *)qname) + 1) + sizeof(struct QUESTION);
    if (sendto(sock, (char *)buf, QuerySize, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0)
    {
        cerr << "Packet Send Failed!!" << endl;
    }

    // Receive the answer
    i = sizeof(dest);

    cout << "\nReceiving DNS Response..." << endl;

    if (recvfrom(sock, (char *)buf, LEN, 0, (struct sockaddr *)&dest, (socklen_t *)&i) < 0)
    {
        cerr << "DNS Response not received!!" << endl;
    }

    cout << "Parsing DNS Response..." << endl;

    dns = (struct DNS_HEADER *)&buf;

    if(dns->rcode == ERR_CODE)
    {
        cerr<<"\nNo such hostname: "<<host_name<<endl;
        cout<<endl;
        return;
    }

    // Point to Answer Section beside DNS header and query field
    ptr = &buf[QuerySize];
    readlen = 0;
    int ans_count = ntohs(dns->ans_count);
    int auth_count = ntohs(dns->auth_count);
    int add_count = ntohs(dns->add_count);
    // Read Answer records
    Read_Answers(ans_count, ptr, host_name);

    // Read Authority records
    Read_Authorities(auth_count, ptr, host_name);

    // Read Additional Records
    Read_Additional(add_count, ptr, host_name);

    Print_Packets(temp_addr, host_name);

    // Add sizes of answers, auth, and addit vectors to dns_cache
    dns_cache[host_name].push_back(make_pair("answers", to_string(answers.size())));
    dns_cache[host_name].push_back(make_pair("auth", to_string(auth.size())));
    dns_cache[host_name].push_back(make_pair("addit", to_string(addit.size())));
}

int main()
{
    unsigned char hostname[100];
    dns_cache.clear();


    cout << " ------------------------- " << endl;
    cout << "| Welcome to DNS Resolver |" << endl;
    cout << " ------------------------- " << endl;

    // Get the hostname from the terminal

    while (true)
    {
        answers.clear();
        auth.clear();
        addit.clear();
        cout << "Enter a hostname to lookup or 'exit' to quit: ";
        fflush(stdout);
        cin >> hostname;
        if (strcmp((char *)hostname, "exit") == 0)
        {
            return 0;
        }
        else
        {
            // Asking for Type-A record
            DNS_Resolver(hostname, T_A);
        }
    }

    return 0;
}