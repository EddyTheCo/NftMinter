#pragma once

#include <QAbstractListModel>
#include<QObject>
#include<QString>
#include <QtQml/qqmlregistration.h>

#include<QHash>

#include "block/qoutputs.hpp"


using namespace  qiota;
using namespace  qblocks;

class NftBox :public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString  issuer READ issuerBech32 WRITE setIssuer NOTIFY issuerChanged)
    Q_PROPERTY(QString  address READ addressBech32 NOTIFY addressChanged)
    Q_PROPERTY(QString  metdata READ metdata WRITE setMetdata NOTIFY metdataChanged)
    Q_PROPERTY(QUrl  uri READ uri  NOTIFY uriChanged)
    Q_PROPERTY(QString  name READ name  NOTIFY nameChanged)
    Q_PROPERTY(Stte  state READ state  NOTIFY stateChanged)
    Q_PROPERTY(bool  selected MEMBER m_selected  NOTIFY selectedChanged)
    QML_ELEMENT

public:
    NftBox(QObject *parent = nullptr):QObject(parent),m_selected(false),m_state(Stte::Ready),m_outId(QString::number(index++)){}
    NftBox(std::shared_ptr<const Output> out, QObject *parent = nullptr,c_array outId=c_array());

    enum Stte {
        Minting,
        Sending,
        Burning,
        Ready
    };
    Q_ENUM(Stte)

    static bool set_addr(QString var_str,c_array& var);
    bool selected()const{return m_selected;}
    void setSelected(bool selected){if(selected!=m_selected){m_selected=selected;emit selectedChanged();}}

    QString metdata()const{return (m_data.isEmpty())?QString():QString(m_data);}
    QString issuerBech32()const{return m_issuerBech32;}
    QString addressBech32()const{return m_addressBech32;}
    c_array addrArray()const{return m_address;}
    c_array issuerArray()const{return m_issuer;}
    c_array dataArray()const{return m_data;}
    c_array outId()const{return m_outId;}
    QString name()const{return m_name;}
    void mint();
    void burn();
    QUrl uri()const{return m_uri;}
    Stte state()const{return m_state;}
    void setState(Stte state){if(state!=m_state){m_state=state;emit stateChanged();}};

    void setMetdata(QString data);
    void setIssuer(QString addr)
    {
        if(NftBox::set_addr(addr,m_issuer))emit issuerChanged();
    }
    pset<const Feature> getFeatures();

signals:
    void issuerChanged();
    void addressChanged();
    void metdataChanged();
    void uriChanged();
    void nameChanged();
    void stateChanged();
    void remove();
    void selectedChanged();

private:


    void fillIRC27(QJsonObject data);
    c_array m_issuer,m_address,m_data,m_outId;
    QString m_addressBech32,m_issuerBech32,m_name;
    QUrl m_uri;
    Stte m_state;
    static size_t index;
    bool m_selected;

};

class BoxModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int newBoxes READ newBoxes NOTIFY newBoxesChanged)
    Q_PROPERTY(int selecteds READ selecteds NOTIFY selectedsChanged)
    QML_ELEMENT
    QML_SINGLETON

public:
    enum ModelRoles {
        issuerRole = Qt::UserRole + 1,
        metdataRole,addressRole,uriRole,nameRole,stateRole,selectedRole};

    int count() const;
    int newBoxes()const{return newBoxes_;}
    BoxModel(QObject *parent = nullptr);
    int selecteds()const{return m_selecteds;}

    Q_INVOKABLE void clearBoxes(bool emptyones=true);

    Q_INVOKABLE void newBox(void)
    {
        addBox(new NftBox(this));
        newBoxes_++;
        emit newBoxesChanged();
    };
    void addBox(NftBox* nbox);
    Q_INVOKABLE void send(QString amount,QString recAddr, QDateTime unixTime)
    {
        if(m_selecteds)
        {
            sendSelecteds(recAddr, unixTime);
            return;
        }
        if(amount!="0")
        {
            sendBT(amount,recAddr, unixTime);
            return;
        }
        sendAll(recAddr);

    };

    Q_INVOKABLE void rmBox(int i);
    Q_INVOKABLE void mint(int i){
        if(boxes.at(i)->addressBech32().isNull())
            boxes.at(i)->mint();

    }
    Q_INVOKABLE void burn(int i){
        if(!boxes.at(i)->addressBech32().isNull())
            boxes.at(i)->burn();
    }
    int idToIndex(c_array outId);

    Q_INVOKABLE bool setProperty(int i, QString role, const QVariant value);


    int rowCount(const QModelIndex &p) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;
    QHash<int, QByteArray> roleNames() const;

signals:
    void countChanged(int count);
    void cIssuerChanged();
    void newBoxesChanged();
    void selectedsChanged();

private:
    void sendSelecteds(QString recAddr, QDateTime unixTime);
    void sendBT(QString amount, QString recAddr, QDateTime unixTime);
    void sendAll(QString recAddr);
    void gotInput(c_array id);
    void lostInput(c_array id);
    int m_count,newBoxes_,m_selecteds;
    QList<NftBox*> boxes;

};






