#include <iostream>
#include <fstream>
#include <windows.h>
#include <string>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>

using namespace std;

//Znaczniki
string ZnacznikiCzasu[2] ;
string ZnacznikiTitle[2] ;
string ZnacznikiText[2] ;
string ZnacznikiZwrotek[2] ;
string ZnacznikEndL ;

/*
    [0] = data
    [1] = title
    [2] = zwortka poczatek
    [3] = zwrotka koniec
    [4] = nazwa zwrotki
    [5] = text
*/
string ZnacznikiTemplatki[6]={"###data###","###title###","###z###","###/z###","###name###","###text###"};
string toClean;

string templatka;
string czas;


string saveDirectory;
string readDirectory;

//dane do zapisu
class Data
{
    public:
        string title;
        string nazwy[20];
        string texty[20];
        int liczbaZwrotek = 0;

        bool ok;

};

class szukajOutput
{
    public:

        string text;
        string context;
        bool znaleziono;

        szukajOutput(bool z, string t, string c)
        {
            znaleziono = z;
            context = c;
            text = t;
        }
};

//------------------

szukajOutput szukaj(string linia, string start , string koniec, bool con)
{
    string out = linia;
    bool znaleziono = false;

    size_t posS = out.find(start);
    if(posS!=string::npos)
    {
        znaleziono=true;
        int posN = posS+start.length();
        int len = linia.length()-posN;

        out = out.substr(posN,len);
        con = true;
    }

    size_t posK = out.find(koniec);
    if(posK!=string::npos)
    {
        znaleziono=true;
        out = out.substr(0,posK);
        con = false;
    }


    if(znaleziono)
    {
        return szukajOutput(con,out,"");
    }
    else
    {
        if(con)
        {
            return szukajOutput(con,"\n"+out,"");
        }
        else
        {
            return szukajOutput(con,"","");
        }

    }

}

szukajOutput szukajZwrotka(string text, string start, string koniec)
{
    string out = text;
    bool znaleziono = false;

    size_t posS = out.find(start);
    if(posS!=string::npos)
    {
        znaleziono=true;
        int posN = posS+start.length();
        int len = text.length()-posN;

        out = out.substr(posN,len);
    }

    string rest="";
    size_t posK = out.find(koniec);
    if(posK!=string::npos)
    {
        rest = out.substr(posK+koniec.length(),out.length());
        znaleziono=true;
        out = out.substr(0,posK);
    }

    return szukajOutput(znaleziono,out,rest);
}

string wytnij(string in, string re)
{
    size_t pos = in.find(re);
    if(pos!=string::npos)
    {
        in=in.substr(0,pos);
    }
    return in;
}

string clean(string text,string toClean)
{
    int n;
    for(int i=0;i<text.length();i++)
    {
        bool nieZnaleziono = true;

        for(int j=0;j<sizeof(toClean)+1;j++)
        {
            if(text[i]==toClean[j])
            {
                nieZnaleziono=false;
            }
        }

        if(nieZnaleziono)
        {
            n=i;
            break;
        }

    }
    text = text.substr(n,text.length());

    int m;
    for(int i=text.length()-1;i>0;i--)
    {
        bool nieZnaleziono = true;

        for(int j=0;j<sizeof(toClean);j++)
        {
            if(text[i]==toClean[j])
            {
                nieZnaleziono=false;
            }
        }

        if(nieZnaleziono)
        {
            m=i;
            break;
        }

    }
    return text = text.substr(0,m+1);

}

string cheange(string text, string from, string to)
{
    size_t pos;
    do
    {
        pos = text.find(from);
        if(pos!=string::npos)
        {
            text.replace(pos,from.length(),to);
        }
    }
    while(pos!=string::npos);

    return text;
}

//--------------------

string getTime()
{

    time_t czas;
    struct tm * data;
    char godzina[19];

    time( & czas );
    data = localtime( & czas );

    strftime( godzina, 20, "%Y-%m-%dT%H:%M:%S", data );

    int i;
    string s = "";
    for (i = 0; i < 19; i++)
    {
        s = s + godzina[i];
    }

    cout<<"Czas i data: "<<s<<endl;

    return s;
}

int getModel()
{
    cout<<"Wczytywanie modelu... ";

    fstream plik;
    string linia;
    plik.open("model.txt",fstream::in);
    if(!plik.good())
    {
        cout << "[Error]"<<endl;
        plik.close();
        return 0;
    }

    int nrlini = 0;
    while(getline(plik,linia))
    {
        switch(nrlini)
        {
            case 0:
                ZnacznikiCzasu[0]=linia;
            break;
            case 1:
                ZnacznikiCzasu[1]=linia;
            break;
            case 2:
                ZnacznikiTitle[0]=linia;
            break;
            case 3:
                ZnacznikiTitle[1]=linia;
            break;
            case 4:
                ZnacznikiText[0]=linia;
            break;
            case 5:
                ZnacznikiText[1]=linia;
            break;
            case 6:
                ZnacznikiZwrotek[0]=linia;
            break;
            case 7:
                ZnacznikiZwrotek[1]=linia;
            break;
            case 8:
                ZnacznikEndL=linia;
            break;
            case 9:
                readDirectory=linia+"\\";
            break;
            case 10:
                saveDirectory=linia+"\\";
            break;
            case 11:
                toClean=linia+"\n";
            break;
        }
        nrlini++;
    }
    cout<<"[OK]"<<endl;
    plik.close();
    return 1;
}

int getTemplate()
{
    cout<<"Wczytywanie templatki... ";
    fstream plik;
    string linia;
    plik.open("template.xml",fstream::in);
    if(!plik.good())
    {
        cout << "[Error]"<<endl;
        plik.close();
        return 0;
    }

    while(getline(plik,linia))
    {
        templatka +=linia;
    }

    cout<<"[OK]"<<endl;
    plik.close();
    return 1;
}

Data calculateXML(string fileName)
{
    Data data;
    data.ok=true;

    string lina;
    fstream plikIn;

    string text ="";

    //otwieranie pliku
    cout<<"Otwieranie pliku: "<<fileName<<" ... ";

    plikIn.open(fileName.c_str(),fstream::in);
    if(!plikIn.good())
    {
        cout << "[Error]"<<endl;
        plikIn.close();

        data.ok=false;
        return data;
    }

    //czytanie
    bool tit=false;
    bool txt=false;

    while(getline(plikIn,lina))
    {

        szukajOutput out = szukaj(lina,ZnacznikiTitle[0],ZnacznikiTitle[1],tit);
        data.title += out.text;
        tit = out.znaleziono;

        out = szukaj(lina,ZnacznikiText[0],ZnacznikiText[1],txt);
        text += out.text;
        txt = out.znaleziono;


    }

    plikIn.close();
    cout<<"[OK]"<<endl;

    //oblicznie
    cout<<"Szukanie danych: {"<<data.title<<"} ";


    //szukanie zwrotek
    szukajOutput objOut = szukajOutput(false,"","");
    do
    {
        objOut = szukajZwrotka(text,ZnacznikiZwrotek[0],ZnacznikiZwrotek[1]);

        string nowName = objOut.text;
        string nowText = wytnij(wytnij(text,objOut.context),ZnacznikiZwrotek[0]+nowName);
        /*
        cout<<endl<<"-------name:------"<<endl<<nowName<<endl;
        cout<<endl<<"-------text:------"<<endl<<nowText<<endl;
        cout<<endl<<"-------bool:"<<objOut.znaleziono<<endl;
        */

        if(objOut.znaleziono)
        {
            if(data.liczbaZwrotek!=0)
            {
                data.texty[data.liczbaZwrotek-1]=cheange(clean(nowText,toClean),"\n",ZnacznikEndL);
            }
            data.nazwy[data.liczbaZwrotek] = nowName;
            text=objOut.context;
        }
        else
        {
            data.texty[data.liczbaZwrotek-1] = cheange(clean(nowName,toClean),"\n",ZnacznikEndL);
        }

        data.liczbaZwrotek++;
    }
    while(objOut.znaleziono);

    data.liczbaZwrotek--;
    cout<<"{"<<data.liczbaZwrotek<<"}  ";


    cout<<"[OK]"<<endl;


    return data;
}

int save(Data data)
{
    cout<<"Przetwarzanie templatki... ";

    string toSave = templatka;
    toSave = cheange(toSave,ZnacznikiTemplatki[0],czas);
    toSave = cheange(toSave,ZnacznikiTemplatki[1],data.title);

    string forZwrotka = szukaj(toSave,ZnacznikiTemplatki[2],ZnacznikiTemplatki[3],true).text;
    string zwrodki;
    for(int i = 0; i<data.liczbaZwrotek;i++)
    {
        string now;
        now = cheange(forZwrotka,ZnacznikiTemplatki[4],data.nazwy[i]);
        zwrodki += cheange(now,ZnacznikiTemplatki[5],data.texty[i]);
    }

    toSave = cheange(toSave,ZnacznikiTemplatki[2]+forZwrotka+ZnacznikiTemplatki[3],zwrodki);
    cout<<"[OK]"<<endl;

    //-----------------

    fstream plikOut;
    string fileName = data.title+"( ).xml";

    cout<<"Zapis jako {"<<fileName<<"} ... ";

    plikOut.open((saveDirectory+fileName).c_str(),fstream::out);
    if(!plikOut.good())
    {
        cout << "[Error]" <<endl;
        plikOut.close();
        return 0;
    }

    plikOut<<toSave;

    cout<<"[OK]"<<endl;

    plikOut.close();
    return 1;
}

//-----------------

int getData()
{
    czas = getTime();

    cout<<endl;
    if(getModel()==0)
    {
        return 0;
    }


    if(getTemplate()==0)
    {
        return 0;
    }

    return 1;
}

int przetworzXml(string fileName)
{
    Data data;


    data = calculateXML(fileName);

    if(!data.ok)
    {
        return 0;
    }

    if(save(data)==0)
    {
        return 0;
    }

    cout<<"Przetworznono: "<<fileName<<endl;


    return 1;

}


//-----------------

void readList(const char *path)
{
    struct dirent *entry;
    DIR *dir = opendir(path);

    if (dir == NULL)
    {
        return;
    }
    while ((entry = readdir(dir)) != NULL)
    {
        string t = entry->d_name;
        cout<<endl;
        przetworzXml(path+t);

    }
    closedir(dir);
}

int main()
{
    if(getData()==0)
    {
        system("pause");
        return 0;
    }

    char cstr[readDirectory.size() + 1];
	strcpy(cstr, readDirectory.c_str());

    readList(cstr);

    system("pause");
    return 0;
}
