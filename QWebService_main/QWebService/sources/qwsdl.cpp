#include "../headers/qwsdl.h"

QWsdl::QWsdl(QObject *parent) :
    QObject(parent)
{
    replyReceived = false;
    errorState = false;
    errorMessage = "";
    webServiceName = "";
    hostname = "";
    hostUrl = "";
    targetNamespace = "";

    workMethodList = new QStringList();
    workMethodParameters = new QMap<int, QMap<QString, QVariant> >();
    methods = new QMap<QString, QSoapMessage *>;
}

QWsdl::QWsdl(QString wsdlFile, QObject *parent) :
    QObject(parent), wsdlFilePath(wsdlFile)
{
    replyReceived = false;
    errorState = false;
    errorMessage = "";
    webServiceName = "";
    hostname = "";
    hostUrl = ""; //wsdlFile;
    targetNamespace = "";

    workMethodList = new QStringList();
    workMethodParameters = new QMap<int, QMap<QString, QVariant> >();
    methods = new QMap<QString, QSoapMessage *>;

    parse();
}

QWsdl::~QWsdl()
{
    delete methods;
    delete workMethodList;
    delete workMethodParameters;
}

void QWsdl::setWsdlFile(QString wsdlFile) // == resetWsdl()
{
    resetWsdl(wsdlFile);
}

QStringList QWsdl::getMethodNames()
{
    QList<QString> tempMethods = methods->keys();
    QStringList result;
    result.append(tempMethods);

    return result;
}

QMap<QString, QSoapMessage *> *QWsdl::getMethods()
{
    return methods;
}

QString QWsdl::getWebServiceName()
{
    return webServiceName;
}

QString QWsdl::getHostname()
{
    return hostname;
}

QUrl QWsdl::getHostUrl()
{
    return hostUrl;
}

QString QWsdl::getTargetNamespace()
{
    return targetNamespace;
}

//QFile QWsdl::getWsdl()
//{

//}

QString QWsdl::getErrorInfo()
{
    return errorMessage;
}

bool QWsdl::isErrorState()
{
    return errorState;
}

void QWsdl::resetWsdl(QString newWsdlPath)
{    
    wsdlFilePath = newWsdlPath;

    methods->clear();
    workMethodList->clear();
    replyReceived = false;
    errorState = false;
    errorMessage = "";
    xmlReader.clear();

    parse();
}

void QWsdl::fileReplyFinished(QNetworkReply *rply)
{
    QNetworkReply *networkReply = rply;
    QByteArray replyBytes;

    replyBytes = (networkReply->readAll());
    QString replyString = convertReplyToUtf(replyBytes);

    wsdlFilePath = "tempWsdl.asmx~";    
    QFile file("tempWsdl.asmx~");
    if (file.exists())
        file.remove();

    if (!file.open(QFile::WriteOnly))
    {
        errorMessage = "Error: cannot write WSDL file from remote location. Reason: " + file.errorString();
        qDebug() << errorMessage;
        errorState = true;
        return;
    }
    else
    {
        file.write(replyString.toUtf8());
    }

    file.close();
    replyReceived = true;
//    emit replyReady(reply);
}

bool QWsdl::parse()
{
    /*
      WARNING!
      Namespace abbreviations are currently hardcoded!
      This needs to be changed before release.
      Real namespaces can be extracted from 'wsdl:definitions' tag attributes.

      Algorithm extracts method names from "types" tags, which is most probably wrong,
      as it should be cross-checked with other tags ("message", etc.)
    */

    if (errorState)
    {
        errorMessage = "WSDL reader is in error state and cannot parse the file.";
        qDebug() << errorMessage;
        return false;
    }

    prepareFile();

    QFile file(wsdlFilePath);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        errorMessage = "Error: cannot read WSDL file: " + wsdlFilePath + ". Reason: " + file.errorString();
        qDebug() << errorMessage;
        errorState = true;
        return false;
    }

    xmlReader.setDevice(&file);
    xmlReader.readNext();

    while (!xmlReader.atEnd())
    {
        if (xmlReader.isStartElement())
        {
            QString tempN = xmlReader.name().toString() ;
            if (tempN == "definitions")
            {
                targetNamespace = xmlReader.attributes().value("targetNamespace").toString();
                hostname = targetNamespace;
                hostUrl = hostname;
                readDefinitions();
            }
            else
            {
                errorMessage = "Error: file does not have WSDL definitions inside!";
                qDebug() << errorMessage;
                errorState = true;
                return false;
            }
        }
        else
        {
            xmlReader.readNext();
        }
    }

    prepareMethods();

    if (!errorState)
        return true;
    else
        return false;
}

void QWsdl::prepareFile()
{
    QUrl filePath(wsdlFilePath);

    if (!QFile::exists(wsdlFilePath) && filePath.isValid())
    {
        hostname = filePath.host();

        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        connect(manager, SIGNAL(finished(QNetworkReply*)),
                this, SLOT(fileReplyFinished(QNetworkReply*)));

        manager->get(QNetworkRequest(filePath));

        forever
        {
            if (replyReceived == true)
                return;
            else
                qApp->processEvents();
        }
    }
}

void QWsdl::readDefinitions()
{
    //EXPERIMENTAL
    // QMap used to supress multiple "tag not supported yet" messages.
    // May actually evolve into more universal tag recognition and handling structure.
    QMap<QString, bool> tagUsed;
    tagUsed.insert("types", false);
    tagUsed.insert("message", false);
    tagUsed.insert("portType", false);
    tagUsed.insert("binding", false);
    tagUsed.insert("service", false);
    tagUsed.insert("documentation", false);
    //END of EXPERIMENTAL

    xmlReader.readNext();
    QString tempName = xmlReader.name().toString();

    while(!xmlReader.atEnd())
    {
        tempName = xmlReader.name().toString();

        if (xmlReader.isEndElement() && (tempName == "definitions"))
        {
            xmlReader.readNext();
            break;
        }
        else if (xmlReader.isEndElement())
        {
            xmlReader.readNext();
            continue;
        }

//        if (!xmlReader.isEndElement())
//        {
        if (tempName == "types")
        {
            readTypes();
//            tagUsed.insert("types", true);
            xmlReader.readNext();
        }
        else if (tempName == "message")
        {
            if (!tagUsed.value("message"))
                readMessages();
            tagUsed.insert("message", true);
            xmlReader.readNext();
        }
        else if (tempName == "portType")
        {
            if (!tagUsed.value("portType"))
                readPorts();
            tagUsed.insert("portType", true);
            xmlReader.readNext();
        }
        else if (tempName == "binding")
        {

            if (!tagUsed.value("binding"))
                readBindings();
            tagUsed.insert("binding", true);
            xmlReader.readNext();
        }
        else if (tempName == "service")
        {
//            if (!tagUsed.value("service"))
                readService();
            tagUsed.insert("service", true);
            xmlReader.readNext();
        }
        else if (tempName == "documentation")
        {
            if (!tagUsed.value("documentation"))
                readDocumentation();
            tagUsed.insert("documentation", true);
            xmlReader.readNext();
        }
        else
        {
            xmlReader.readNext();
        }
//    }
//        else
//        {
//            xmlReader.readNext();
//        }
    }
}

void QWsdl::readTypes()
{
    xmlReader.readNext();
    xmlReader.readNext();
    QString tempName = xmlReader.name().toString();
    if (xmlReader.name().toString() == "schema")
    {
        xmlReader.readNext();
    }
    else
    {
        errorMessage = "Error: file does not have WSDL schema tag inside!";
        qDebug() << errorMessage;
        errorState = true;
        return;
    }

    QString lastName = "";
    tempName = "";
    while(!xmlReader.atEnd())
    {
        if (tempName != "")
            lastName = tempName;
        tempName = xmlReader.name().toString();

        if (tempName == "element" && (xmlReader.attributes().count() == 1))
        {
            QString elementName = xmlReader.attributes().value("name").toString();
            workMethodList->append(elementName);
            readTypeSchemaElement();
        }
        else if (xmlReader.isEndElement() && (tempName == "schema"))
        {
            xmlReader.readNext();
            break;
        }
        else
        {
            xmlReader.readNext();
        }
    }
}

void QWsdl::readTypeSchemaElement()
{
    xmlReader.readNext();
    QMap<QString, QVariant> params;

    bool firstElem = true;
    QString tempName = ""; //xmlReader.name().toString();
    QString lastName = "";
    while(!xmlReader.atEnd())
    {
        if (tempName != "")
            lastName = tempName;
        tempName = xmlReader.name().toString();

        if (xmlReader.isEndElement()
                && (xmlReader.name() == "element")
                && ((lastName == "complexType") || (lastName == "sequence"))
                && (firstElem == false)
                )
        {
            int currentMethodIndex = workMethodList->length() - 1;
            workMethodParameters->insert(currentMethodIndex, params);
            xmlReader.readNext();
            break;
        }

        if ((tempName == "complexType") || (tempName == "sequence"))
        {
            firstElem = false;
            xmlReader.readNext();
            continue;
        }

        if (tempName == "element")
        {
            firstElem = false;
            // Min and max occurences are not taken into account!
            QString elementName = xmlReader.attributes().value("name").toString();
            // Ommits namespace ("s:int" => "int")
            QString elementType = xmlReader.attributes().value("type").toString();

            if ((elementName == "") || (elementType == "") )
            {
                xmlReader.readNext();
                continue;
            }

            QVariant element;
            elementType = elementType.split(':').at(1);

            if (elementType == "int")
            {
                element.setValue(0);
            }
            else if (elementType == "boolean")
            {
                element.setValue(true);
            }
            else if (elementType == "dateTime")
            {
                element.setValue(QDateTime());
            }
            else if (elementType == "string")
            {
                element.setValue(QString(""));
            }
            else if (elementType == "ArrayOfString")
            {
                element.setValue(QStringList());
            }
            else
            {
                element.setValue(QString("temp"));
            }// NEEDS MANY MORE VALUE TYPES! VERY SHAKY IMPLEMENTATION!

            params.insert(elementName, element);
            xmlReader.readNext();
        }
        else
        {
            xmlReader.readNext();
        }
    }
}

/*
 * Analyses both "working" QList and QMap, and extracts methods data, which is then put into
 * 'methods' QMap.
 */
void QWsdl::prepareMethods()
{
    if (errorState)
        return;

    bool methodsDone[workMethodList->length()];

    for (int x = 0; x < workMethodList->length(); x++)
        methodsDone[x] = false;


    for (int i = 0; (i < workMethodList->length()); i++)
    {
        if (methodsDone[i] == false)
        {
            bool isMethodAndResponsePresent = false;
            int methodMain = 0;
            int methodReturn = 0;
            QString methodName = workMethodList->at(i);

            if (methodName.contains("Response"))
            {
                methodReturn = i;
                methodsDone[i] = true;
                QString tempMethodName = methodName;
                tempMethodName.chop(8);

                for (int j = 0; j < workMethodList->length(); j++)
                {
                    if (workMethodList->at(j) == tempMethodName)
                    {
                        methodMain = j;
                        methodsDone[j] = true;
                        isMethodAndResponsePresent = true;
                        methodName = tempMethodName;
                        break;
                    }
                }
            }
            else
            {
                methodMain = i;

                for (int j = 0; j < workMethodList->length(); j++)
                {
                    if (workMethodList->at(j) == (methodName + "Response"))
                    {
                        methodReturn = j;
                        methodsDone[j] = true;
                        isMethodAndResponsePresent = true;
                        break;
                    }
                }
            }

            if (isMethodAndResponsePresent == true)
            {
                methods->insert(methodName, new QSoapMessage(targetNamespace,
                                                             methodName,
                                                             workMethodParameters->value(methodMain),
                                                             workMethodParameters->value(methodReturn)));
                methods->value(methodName)->setTargetNamespace(targetNamespace);
            }
        }
    }
}

void QWsdl::readMessages()
{
    qDebug() << "WSDL :messages tag not supported yet.";
    xmlReader.readNext();
}

void QWsdl::readPorts()
{
    qDebug() << "WSDL :portType tag not supported yet.";
    xmlReader.readNext();
}

void QWsdl::readBindings()
{
    qDebug() << "WSDL :binding tag not supported yet.";
    xmlReader.readNext();
}

void QWsdl::readService()
{
//    qDebug() << "WSDL :service tag not supported yet.";
//    xmlReader.readNext(); xmlReader.readNext();
    if (webServiceName == "")
        webServiceName = xmlReader.attributes().value("name").toString();
//    qDebug() << "Web service's name is now set.";
    xmlReader.readNext();
}

void QWsdl::readDocumentation()
{
    qDebug() << "WSDL :documentation tag not supported yet.";
    xmlReader.readNext();
}

QString QWsdl::convertReplyToUtf(QString textToConvert)
{
    QString result = textToConvert;

    result.replace("&lt;", "<");
    result.replace("&gt;", ">");

    return result;
}
