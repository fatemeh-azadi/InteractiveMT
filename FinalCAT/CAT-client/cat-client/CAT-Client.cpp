
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <iostream>
#include "../client.h"
#include <QString>
#include <QTextStream>
#include <QFile>
using namespace std;

Client *client;
int port;
void process(QString s, QString ref, int &k, int &m, QTextStream &fout) {

    k = 0;
    m = 0;
//    QTextStream qtout(stdout);
//    qtout.setCodec("UTF-8");

    client->set_message("new_sentence");
    fout << "source: " << s << endl;
    fout << "reference: " << ref << endl << endl;
    if (strcmp(client->get_message(), "ok") == 0) {

        client->set_message(s.toUtf8());

        QString prefix , res ;

        int idx = 0;
        while (idx < ref.length()) {
            prefix = ref.mid(0, idx);

            fout << "prefix: " << prefix << endl;
            if(idx > 0){
                client->set_message("prefix");
//                fout << "prefix sent" << endl;
            }
            else
                client->set_message("no-prefix");
            string x = client->get_message();
            if (strcmp(x.c_str(), "ok") == 0) {
//                fout << "ok " << idx << endl;
                if(idx > 0) client->set_message(prefix.toUtf8());

                res = res.fromUtf8(client->get_message(), -1);

                fout << "Translation: " << res << endl;
            } else
                fout << "server not respond to prefix" << endl;
            int i = 0;
            while (idx < ref.length() && i < res.length()
                    && ref[idx] == res[i]){
                idx++;
                i++;
            }
//            fout << ref[idx] << " " << res[i] << endl;
            if(i > 0) m++;
            k++;
            idx++;

        }

    } else {
        fout << "server not respond" << endl;
    }

    fout << "Update Weights..." << endl;
    client->set_message("update");
    if (strcmp(client->get_message(), "ok") == 0) {

        client->set_message(ref.toUtf8());
        if (strcmp(client->get_message(), "ok") != 0)
            fout << "server not update ref" << endl;
    }else
        fout << "server not update" << endl;
}



int main(int argc, char* argv[]) {

    string inputFile = "", referenceFile = "", output = "";
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "--input") == 0 || strcmp(argv[i], "-i") == 0) {
            inputFile = argv[i + 1];
            cout << argv[i + 1] << endl;
        } else if (strcmp(argv[i], "--reference") == 0
                || strcmp(argv[i], "-r") == 0)
            referenceFile = argv[i + 1];
        else if (strcmp(argv[i], "--output") == 0 || strcmp(argv[i], "-o") == 0)
            output = argv[i + 1];
        else if (strcmp(argv[i], "--port") == 0 || strcmp(argv[i], "-p") == 0)
            port = atoi(argv[i + 1]);
    }

    client = new Client("127.0.0.1", port);

    QString source = "";
    QString ref = "";
    int ks = 0, mo = 0, id = 0;
    int total_chars = 0;
    QFile input_file(inputFile.c_str());
    QFile ref_file(referenceFile.c_str());
    QFile out_file(output.c_str());
    if( !input_file.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        return 1;
    }
    if (!ref_file.open(QIODevice::ReadOnly | QIODevice::Text))
            return 1;
    if (!out_file.open(QIODevice::ReadWrite | QIODevice::Text))
        return 1;

    QTextStream fin_input(&input_file);
    QTextStream fin_ref(&ref_file);
    QTextStream fout(&out_file);

    fin_ref.setCodec("UTF-8");
    fin_input.setCodec("UTF-8");
    fout.setCodec(("UTF-8"));

    while (!fin_input.atEnd()) {
        source = fin_input.readLine(); // here you have a line from text file stored in QString
        ref = fin_ref.readLine();
        int k, m;

        fout << "Translating Sentence " << id++ << " :" << endl;
        process(source, ref, k, m, fout);
        total_chars += ref.length();
        fout << "ksr: " << k << "  mar: " << m << endl << endl;
        ks += k;
        mo += m;
    }

    client->set_message("close");
    fout << "Total KSR: " << ks << endl;
    fout << "Total MAR: " << mo << endl;
    fout << "Total Reference Characters: " << total_chars << endl;
    fout.flush();

    return 0;
}


