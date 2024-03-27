#include"CreateNFT.hpp"
#include<QDataStream>
#include<QJsonDocument>
#include<QTimer>

#define USE_QML
#include "encoding/qbech32.hpp"
#include"nodeConnection.hpp"
#include "qaddr_bundle.hpp"
#include "qwallet.hpp"
#include "account.hpp"

size_t NftBox::index=0;
bool NftBox::set_addr(QString var_str,c_array& var)
{
    const auto addr_pair=qencoding::qbech32::Iota::decode(var_str);
    if(addr_pair.second.size()&&addr_pair.second!=var)
    {
        var=addr_pair.second;
        return true;
    }
    return false;

}
NftBox::NftBox(std::shared_ptr<const Output> out, QObject *parent, c_array outId):NftBox(parent)
{
    m_outId=outId;
    auto info=NodeConnection::instance()->rest()->get_api_core_v2_info();
    connect(info,&Node_info::finished,this,[=]( ){
        const auto issuefea=out->get_immutable_feature_(qblocks::Feature::Issuer_typ);
        if(issuefea)
        {
            m_issuer=std::static_pointer_cast<const Issuer_Feature>(issuefea)->issuer()->addr();
            m_issuerBech32=qencoding::qbech32::Iota::encode(info->bech32Hrp,m_issuer);
            emit issuerChanged();
        }
        const auto immetfea=out->get_immutable_feature_(qblocks::Feature::Metadata_typ);
        if(immetfea)
        {
            m_data=std::static_pointer_cast<const Metadata_Feature>(immetfea)->data();
            auto var3=QJsonDocument::fromJson(m_data);


            if(!var3.isNull())
            {
                m_data=var3.toJson(QJsonDocument::Indented);
                fillIRC27(var3.object());
            }
            emit metdataChanged();
        }
        const auto id=out->get_id();
        m_address=Address::NFT(id)->addr();
        m_addressBech32=qencoding::qbech32::Iota::encode(info->bech32Hrp,m_address);
        emit addressChanged();
        info->deleteLater();
    });

}
pset<const Feature> NftBox::getFeatures()
{
    pset<const Feature> features;
    if(!m_issuer.isNull())
    {
        const auto issuer=Feature::Issuer(Address::from_array(m_issuer));
        features.insert(issuer);
    }
    if(!m_data.isNull())
    {
        const auto metadata=Feature::Metadata(m_data);
        features.insert(metadata);
    }
    return features;
}
void NftBox::mint()
{

    setState(Minting);
    QTimer::singleShot(20000,this,[=](){setState(Ready);});
    auto info=NodeConnection::instance()->rest()->get_api_core_v2_info();
    connect(info,&Node_info::finished,this,[=]( ){

        if(Wallet::instance()->addresses().size())
        {

            const auto unlock=
                Unlock_Condition::Address(Wallet::instance()->addresses().begin()->second->getAddress());

            auto features=getFeatures();

            auto NFTout=Output::NFT(0,{unlock},{},features);
            const auto deposit=NodeConnection::instance()->rest()->get_deposit(NFTout,info);
            NFTout->amount_=deposit;
            pvector<const Output> theOutputs{NFTout};

            if(m_issuer.isNull()||Wallet::instance()->addresses().find(m_issuer)!=Wallet::instance()->addresses().cend())
            {
                InputSet inputSet;

                StateOutputs stateOutputs1;
                quint64 consumedAmount=0;
                if(!m_issuer.isNull())
                {
                    const auto addB=Wallet::instance()->addresses().at(m_issuer);
                    const auto issuerAddr=addB->getAddress();

                    if(issuerAddr->type()==Address::NFT_typ||issuerAddr->type()==Address::Alias_typ)
                    {
                        consumedAmount+=Wallet::instance()->
                                          consume(inputSet,stateOutputs1,deposit,{Output::All_typ},{addB->outId()});
                    }
                }

                quint64 stateAmount=0;
                for(const auto &v:std::as_const(stateOutputs1))
                {
                    auto out=v.output->clone();
                    auto prevUnlock=out->unlock_conditions_;
                    out->consume();
                    out->unlock_conditions_={prevUnlock};
                    const auto cdep=NodeConnection::instance()->rest()->get_deposit(out,info);
                    out->amount_=cdep;
                    stateAmount+=cdep;
                    theOutputs.push_back(out);
                }
                quint64 requiredAmount=deposit+stateAmount;
                StateOutputs stateOutputs2;
                if(consumedAmount<requiredAmount)
                {
                    consumedAmount+=Wallet::instance()->
                                      consume(inputSet,stateOutputs2,0,{Output::Basic_typ},{}); //Fix this set the amount need it
                }


                for(const auto &v:std::as_const(stateOutputs2))
                {
                    auto out=v.output->clone();
                    auto prevUnlock=out->unlock_conditions_;
                    out->consume();
                    out->unlock_conditions_={prevUnlock};
                    const auto cdep=NodeConnection::instance()->rest()->get_deposit(out,info);
                    out->amount_=cdep;
                    stateAmount+=cdep;
                    theOutputs.push_back(out);
                }
                requiredAmount=deposit+stateAmount;

                if(consumedAmount>=requiredAmount)
                {
                    auto BaOut=Output::Basic(0,{unlock});
                    const auto minDeposit=Client::get_deposit(BaOut,info);
                    if(consumedAmount-requiredAmount>minDeposit)
                    {

                        BaOut->amount_=consumedAmount-requiredAmount;
                        theOutputs.push_back(BaOut);
                    }
                    else
                    {
                        NFTout->amount_+=consumedAmount-requiredAmount;
                    }
                    auto payloadusedids=Wallet::instance()->createTransaction(inputSet,info,theOutputs);
                    auto block=Block(payloadusedids.first);

                    const auto transactionid=payloadusedids.first->get_id().toHexString();
                    auto res=NodeConnection::instance()->mqtt()->get_subscription("transactions/"+transactionid +"/included-block");
                    QObject::connect(res,&ResponseMqtt::returned,this,[=](auto var){
                        emit remove();
                        res->deleteLater();
                    });

                    NodeConnection::instance()->rest()->send_block(block);

                }
                else
                {

                    this->setState(Ready);
                }
            }
            else
            {
                setState(Ready);
            }

        }

        info->deleteLater();
    });
}
void NftBox::burn()
{
    setState(Burning);
    QTimer::singleShot(20000,this,[=](){setState(Ready);});
    auto info=NodeConnection::instance()->rest()->get_api_core_v2_info();
    connect(info,&Node_info::finished,this,[=]( ){
        if(Wallet::instance()->addresses().find(m_address)!=Wallet::instance()->addresses().cend())
        {
            const auto unlock=
                Unlock_Condition::Address(Wallet::instance()->addresses().begin()->second->getAddress());

            const auto maininputs=Wallet::instance()->addresses().at(m_address)->inputs();
            std::set<c_array> outids;
            for (auto i = maininputs.cbegin(), end = maininputs.cend(); i != end; ++i)
            {
                outids.insert(i.key());
            }
            outids.insert(m_outId);
            pvector<const Output> theOutputs;
            InputSet inputSet;
            StateOutputs stateOutputs1;
            quint64 consumedAmount=0;
            quint64 stateAmount=0;
            consumedAmount+=Wallet::instance()->
                              consume(inputSet,stateOutputs1,0,{Output::NFT_typ},outids);

            for (auto i = stateOutputs1.cbegin(), end = stateOutputs1.cend(); i != end; ++i)
            {

                auto out=i.value().output->clone();
                out->consume();
                if(i.key()!=m_outId)
                {

                    out->unlock_conditions_={unlock};    //Fix: Add state transition is if alias
                    const auto cdep=NodeConnection::instance()->rest()->get_deposit(out,info);
                    out->amount_=cdep;
                    stateAmount+=cdep;
                    theOutputs.push_back(out);
                }
            }
            auto BaOut=Output::Basic(0,{unlock});
            const auto minDeposit=Client::get_deposit(BaOut,info);
            StateOutputs stateOutputs2;
            if(consumedAmount<stateAmount+minDeposit)
            {
                consumedAmount+=Wallet::instance()->
                                  consume(inputSet,stateOutputs2,0,{Output::Basic_typ},{}); //Fix this set the amount need it
            }
            for(const auto &v:std::as_const(stateOutputs2))
            {
                auto out=v.output->clone();
                auto prevUnlocks=out->unlock_conditions_;
                out->consume();
                out->unlock_conditions_=prevUnlocks;    //Fix: Add state transition is if alias
                const auto cdep=NodeConnection::instance()->rest()->get_deposit(out,info);
                out->amount_=cdep;
                stateAmount+=cdep;
                theOutputs.push_back(out);
            }

            if(consumedAmount>=stateAmount+minDeposit)
            {

                BaOut->amount_=consumedAmount-stateAmount;
                theOutputs.push_back(BaOut);

                auto payloadusedids=Wallet::instance()->createTransaction(inputSet,info,theOutputs);
                auto block=Block(payloadusedids.first);

                const auto transactionid=payloadusedids.first->get_id().toHexString();
                auto res=NodeConnection::instance()->mqtt()->get_subscription("transactions/"+transactionid +"/included-block");
                QObject::connect(res,&ResponseMqtt::returned,this,[=](auto var){
                    emit remove();
                    res->deleteLater();
                });

                NodeConnection::instance()->rest()->send_block(block);
            }
            else
            {
                setState(Ready);
            }
        }
        else
        {
            setState(Ready);
        }
        info->deleteLater();
    });
}
void NftBox::fillIRC27(QJsonObject data)
{

    const auto name=data["name"].toString();
    if(!name.isNull()&&name!=m_name)
    {
        m_name=name;
        emit nameChanged();
    }
    auto uri=QUrl(data["uri"].toString());
    if(uri.scheme()=="ipfs")
    {
        const auto id=uri.host();
        uri=QUrl("https://nftstorage.link/ipfs/"+id);
    }
    if(uri.isValid()&&uri!=m_uri)
    {
        m_uri=uri;
        emit uriChanged();
    }

}
void NftBox::setMetdata(QString data)
{
    auto var=QJsonDocument::fromJson(data.toUtf8());
    c_array vararr;
    if(!var.isNull())
    {
        vararr=var.toJson(QJsonDocument::Compact);
    }
    else
    {
        vararr=c_array(data.toUtf8());
    }
    if(vararr!=m_data)
    {
        m_data=vararr;
        if(!var.isNull())
        {
            fillIRC27(var.object());
        }
        emit metdataChanged();
    }

};
BoxModel::BoxModel(QObject *parent)
    : QAbstractListModel(parent),newBoxes_(0),m_selecteds(0){

    Account::instance()->setVaultFile(
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+"/NFTMinter/qvault.bin");
    connect(Wallet::instance(),&Wallet::synced,this,[=](){clearBoxes(false);});
    connect(Wallet::instance(),&Wallet::inputAdded,this,&BoxModel::gotInput);
    connect(Wallet::instance(),&Wallet::inputRemoved,this,[=](c_array id)
            {
                auto ind=idToIndex(id);
                if(ind>-1)rmBox(ind);
            });
}
void BoxModel::gotInput(c_array id)
{
    const auto inBox=Wallet::instance()->getInput(id);
    const auto output=inBox.output;

    if(output->type()==qblocks::Output::NFT_typ)
    {
        auto nbox=new NftBox(output,this,id);
        addBox(nbox);
    }

}
void BoxModel::sendAll(QString recAddr)
{
    auto info=NodeConnection::instance()->rest()->get_api_core_v2_info();
    connect(info,&Node_info::finished,this,[=]( ){
        c_array recArray;
        if(NftBox::set_addr(recAddr,recArray))
        {
            const auto addrUnlock=Unlock_Condition::Address(Address::from_array(recArray));
            pvector<const Output> theOutputs;
            const auto maininputs=Wallet::instance()->addresses().begin()->second->inputs();
            std::set<c_array> outids;
            for (auto i = maininputs.cbegin(), end = maininputs.cend(); i != end; ++i)
            {
                outids.insert(i.key());
            }
            for(auto i=0;i<boxes.size();i++)
            {
                const auto box=boxes[i];
                outids.insert(box->outId());
                box->setState(NftBox::Stte::Sending);
                QTimer::singleShot(20000,box,[=](){box->setState(NftBox::Ready);});
            }
            InputSet inputSet;
            StateOutputs stateOutputs1;
            quint64 consumedAmount=0;
            consumedAmount+=Wallet::instance()->
                              consume(inputSet,stateOutputs1,0,{Output::All_typ},outids);

            quint64 stateAmount=0;
            for (auto i = stateOutputs1.cbegin(), end = stateOutputs1.cend(); i != end; ++i)
            {
                auto out=i.value().output->clone();
                auto prevUnlock=out->unlock_conditions_;
                out->consume();
                out->unlock_conditions_={addrUnlock};    //Fix: Add state transition is if alias

                const auto cdep=NodeConnection::instance()->rest()->get_deposit(out,info);
                out->amount_=cdep;
                stateAmount+=cdep;
                theOutputs.push_back(out);
            }
            StateOutputs stateOutputs2;
            auto BaOut=Output::Basic(0,{addrUnlock});
            const auto minDeposit=Client::get_deposit(BaOut,info);
            if(consumedAmount<stateAmount+minDeposit)
            {
                consumedAmount+=Wallet::instance()->
                                  consume(inputSet,stateOutputs2,0,{Output::Basic_typ},{}); //Fix this set the amount need it
            }

            for(const auto &v:std::as_const(stateOutputs2))
            {
                auto out=v.output->clone();
                auto prevUnlocks=out->unlock_conditions_;
                out->consume();
                out->unlock_conditions_=prevUnlocks;    //Fix: Add state transition is if alias
                const auto cdep=NodeConnection::instance()->rest()->get_deposit(out,info);
                out->amount_=cdep;
                stateAmount+=cdep;
                theOutputs.push_back(out);
            }

            if(consumedAmount>=stateAmount+minDeposit)
            {

                BaOut->amount_=consumedAmount-stateAmount;
                theOutputs.push_back(BaOut);

                auto payloadusedids=Wallet::instance()->createTransaction(inputSet,info,theOutputs);
                auto block=Block(payloadusedids.first);

                const auto transactionid=payloadusedids.first->get_id().toHexString();
                auto res=NodeConnection::instance()->mqtt()->get_subscription("transactions/"+transactionid +"/included-block");
                QObject::connect(res,&ResponseMqtt::returned,this,[=](auto var){
                    res->deleteLater();
                });

                NodeConnection::instance()->rest()->send_block(block);

            }
            else
            {
                for(auto i=0;i<boxes.size();i++)
                {
                    if(boxes[i]->state()==NftBox::Stte::Sending)
                    {
                        boxes[i]->setState(NftBox::Stte::Ready);
                    }
                }
            }
        }
        info->deleteLater();
    });
}
void BoxModel::sendSelecteds(QString recAddr, QDateTime unixTime)
{
    auto info=NodeConnection::instance()->rest()->get_api_core_v2_info();
    connect(info,&Node_info::finished,this,[=]( ){

        c_array recArray;
        if(NftBox::set_addr(recAddr,recArray))
        {
            const auto addrUnlock=Unlock_Condition::Address(Address::from_array(recArray));
            const auto unlock=
                Unlock_Condition::Address(Wallet::instance()->addresses().begin()->second->getAddress());
            pset<const Unlock_Condition> theUnlocks{addrUnlock};
            if(unixTime.isValid())
            {
                const auto expirUnlock=Unlock_Condition::Expiration(unixTime.toSecsSinceEpoch(),
                                                                      Wallet::instance()->addresses().begin()->second->getAddress());
                //Add storage unlock to recieve the extra from adding the expiration?
                theUnlocks.insert(expirUnlock);
            }
            pvector<const Output> theOutputs;
            std::set<c_array> outids;
            std::set<c_array> soutids;
            std::set<c_array> routids;

            for(auto i=0;i<boxes.size();i++)
            {
                if(boxes[i]->selected())
                {
                    const auto box=boxes[i];
                    outids.insert(box->outId());
                    soutids.insert(box->outId());
                    box->setState(NftBox::Stte::Sending);
                    const auto maininputs=Wallet::instance()->addresses().at(box->addrArray())->inputs();
                    for (auto i = maininputs.cbegin(), end = maininputs.cend(); i != end; ++i)
                    {
                        outids.insert(i.key());
                        routids.insert(i.key());
                    }
                    QTimer::singleShot(20000,box,[=](){box->setState(NftBox::Ready);});
                }
            }

            InputSet inputSet;
            StateOutputs stateOutputs1;
            quint64 consumedAmount=0;
            consumedAmount+=Wallet::instance()->
                              consume(inputSet,stateOutputs1,0,{Output::NFT_typ},outids);

            quint64 stateAmount=0;
            std::shared_ptr<Output> last;
            for (auto i = stateOutputs1.cbegin(), end = stateOutputs1.cend(); i != end; ++i)
            {
                auto out=i.value().output->clone();
                auto prevUnlock=out->unlock_conditions_;
                out->consume();
                if(soutids.find(i.key())!=soutids.cend())
                {
                    out->unlock_conditions_=theUnlocks;
                }
                else
                {
                    if(routids.find(i.key())!=routids.cend())
                    {
                        out->unlock_conditions_={unlock};     //Add state transition is if alias
                    }
                    else
                    {
                        out->unlock_conditions_=prevUnlock;   //Add state transition is if alias
                    }
                }
                const auto cdep=NodeConnection::instance()->rest()->get_deposit(out,info);
                out->amount_=cdep;
                stateAmount+=cdep;
                theOutputs.push_back(out);
                last=out;
            }

            auto BaOut=Output::Basic(0,{unlock});
            const auto minDeposit=Client::get_deposit(BaOut,info);
            if(consumedAmount<stateAmount+minDeposit)
            {
                StateOutputs stateOutputs2;
                consumedAmount+=Wallet::instance()->
                                  consume(inputSet,stateOutputs2,0,{Output::Basic_typ},{}); //Fix this set the amount need it
                for(const auto &v:std::as_const(stateOutputs2))
                {
                    auto out=v.output->clone();
                    auto prevUnlocks=out->unlock_conditions_;
                    out->consume();
                    out->unlock_conditions_=prevUnlocks;   //Fix: Add state transition is if alias
                    const auto cdep=NodeConnection::instance()->rest()->get_deposit(out,info);
                    out->amount_=cdep;
                    stateAmount+=cdep;
                    theOutputs.push_back(out);
                }
            }

            if(consumedAmount>=stateAmount)
            {
                if(consumedAmount-stateAmount>minDeposit)
                {
                    BaOut->amount_=consumedAmount-stateAmount;
                    theOutputs.push_back(BaOut);
                }
                else
                {
                    last->amount_+=consumedAmount-stateAmount;
                }

                auto payloadusedids=Wallet::instance()->createTransaction(inputSet,info,theOutputs);
                auto block=Block(payloadusedids.first);

                NodeConnection::instance()->rest()->send_block(block);

            }
            else
            {
                for(auto i=0;i<boxes.size();i++)
                {
                    if(boxes[i]->state()==NftBox::Stte::Sending)
                    {
                        boxes[i]->setState(NftBox::Stte::Ready);
                    }
                }
            }

        }
        info->deleteLater();
    });
}
void BoxModel::sendBT(QString amountStr, QString recAddr, QDateTime unixTime)
{

    auto info=NodeConnection::instance()->rest()->get_api_core_v2_info();
    connect(info,&Node_info::finished,this,[=]( ){

        c_array recArray;
        if(NftBox::set_addr(recAddr,recArray))
        {
            const auto addrUnlock=Unlock_Condition::Address(Address::from_array(recArray));
            const auto unlock=
                Unlock_Condition::Address(Wallet::instance()->addresses().begin()->second->getAddress());
            pset<const Unlock_Condition> theUnlocks{addrUnlock};
            auto expirUnlock=Unlock_Condition::Expiration(unixTime.toSecsSinceEpoch(),
                                                                  Wallet::instance()->addresses().begin()->second->getAddress());
            if(unixTime.isValid())
            {
                //Add storage unlock to recieve the extra from adding the expiration?
                theUnlocks.insert(expirUnlock);
            }
            pvector<const Output> theOutputs;

            const quint64 amount=amountStr.toULongLong();


            auto BaOut=Output::Basic(0,{unlock});
            auto minDeposit=Client::get_deposit(BaOut,info);

            quint64 neededAmount=(amount>=minDeposit)?amount:(minDeposit+amount);


            if(amount<minDeposit)
            {
                const auto varstoraUnlock=
                    Unlock_Condition::Storage_Deposit_Return( Wallet::instance()->addresses().begin()->second->getAddress(), minDeposit );

                pset<const Unlock_Condition> varUnlocks{addrUnlock,varstoraUnlock};
                if(!unixTime.isValid())
                {
                    expirUnlock=Unlock_Condition::Expiration(QDateTime::currentDateTime().addDays(1).toSecsSinceEpoch(),
                                                                          Wallet::instance()->addresses().begin()->second->getAddress());

                    theUnlocks.insert(expirUnlock);

                }
                varUnlocks.insert(expirUnlock);

                auto VarOut=Output::Basic(0,varUnlocks);

                minDeposit=Client::get_deposit(VarOut,info);
                const auto storaUnlock=
                    Unlock_Condition::Storage_Deposit_Return( Wallet::instance()->addresses().begin()->second->getAddress(), minDeposit-amount );
                theUnlocks.insert(storaUnlock);

            }

            auto SendOut=Output::Basic(0,theUnlocks);
            const auto sendamount=Client::get_deposit(SendOut,info);

            if(neededAmount<sendamount)neededAmount=sendamount;
            SendOut->amount_=neededAmount;

            theOutputs.push_back(SendOut);

            InputSet inputSet;
            StateOutputs stateOutputs1;
            quint64 consumedAmount=0;
            consumedAmount+=Wallet::instance()->
                              consume(inputSet,stateOutputs1,0,{Output::Basic_typ});

            quint64 stateAmount=0;
            for (auto i = stateOutputs1.cbegin(), end = stateOutputs1.cend(); i != end; ++i)
            {
                auto out=i.value().output->clone();
                auto prevUnlock=out->unlock_conditions_;
                out->consume();
                out->unlock_conditions_=prevUnlock;   //Add state transition is if alias
                const auto cdep=NodeConnection::instance()->rest()->get_deposit(out,info);
                out->amount_=cdep;
                stateAmount+=cdep;
                theOutputs.push_back(out);
            }


            if(consumedAmount>=stateAmount+neededAmount)
            {
                if(consumedAmount-stateAmount-neededAmount>minDeposit)
                {
                    BaOut->amount_=consumedAmount-stateAmount-neededAmount;
                    theOutputs.push_back(BaOut);
                }
                else
                {
                    SendOut->amount_+=consumedAmount-stateAmount-neededAmount;
                }

                auto payloadusedids=Wallet::instance()->createTransaction(inputSet,info,theOutputs);
                auto block=Block(payloadusedids.first);

                NodeConnection::instance()->rest()->send_block(block);

            }

        }
        info->deleteLater();
    });
}
int BoxModel::count() const
{
    return boxes.size();
}
void BoxModel::clearBoxes(bool emptyones)
{
    size_t j=0;
    const auto var=boxes.size();
    for (auto i=0;i<var;i++)
    {

        if(boxes[j]->addrArray().isNull()==emptyones)
        {
            rmBox(j);
        }
        else
        {
            j++;
        }
    }
}

int BoxModel::rowCount(const QModelIndex &p) const
{
    Q_UNUSED(p)
    return boxes.size();
}
QHash<int, QByteArray> BoxModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[issuerRole] = "issuer";
    roles[metdataRole] = "metdata";
    roles[addressRole] = "address";
    roles[uriRole] = "uri";
    roles[nameRole] = "name";
    roles[stateRole] = "state";
    roles[selectedRole] = "selected";
    return roles;
}
QVariant BoxModel::data(const QModelIndex &index, int role) const
{
    return boxes[index.row()]->property(roleNames().value(role));
}
bool BoxModel::setData(const QModelIndex &index, const QVariant &value, int role )
{

    if(index.row()<boxes.size())
    {
        const auto re=boxes[index.row()]->setProperty(roleNames().value(role),value);

        if(re)
        {
            emit dataChanged(index,index,QList<int>{role});
            return true;
        }
    }
    return false;
}
bool BoxModel::setProperty(int i,QString role,const QVariant value)
{
    const auto ind=index(i);
    const auto rol=roleNames().keys(role.toUtf8());
    return setData(ind,value,rol.front());
}
QModelIndex BoxModel::index(int row, int column , const QModelIndex &parent ) const
{
    return createIndex(row,column);
}
void BoxModel::addBox(NftBox* o)
{

    int i = boxes.size();
    connect(o,&NftBox::selectedChanged,this,[=](){
        if(o->selected())m_selecteds++;
        else m_selecteds--;
        emit selectedsChanged();
    });
    connect(o,&NftBox::remove,this,[=](){
        o->setSelected(false);
        const auto ind=idToIndex(o->outId());
        if(ind>-1)this->rmBox(ind);
    });
    connect(o,&NftBox::issuerChanged,this,[=]{
        const auto ind=idToIndex(o->outId());
        if(ind>-1)
            emit dataChanged(index(ind),index(ind),QList<int>{ModelRoles::issuerRole});
    });
    connect(o,&NftBox::metdataChanged,this,[=]{
        const auto ind=idToIndex(o->outId());
        if(ind>-1)
            emit dataChanged(index(ind),index(ind),QList<int>{ModelRoles::metdataRole});
    });
    connect(o,&NftBox::addressChanged,this,[=]{
        o->setSelected(false);
        const auto ind=idToIndex(o->outId());
        if(ind>-1)
            emit dataChanged(index(ind),index(ind),QList<int>{ModelRoles::addressRole});
    });
    connect(o,&NftBox::nameChanged,this,[=]{
        const auto ind=idToIndex(o->outId());
        if(ind>-1)
            emit dataChanged(index(ind),index(ind),QList<int>{ModelRoles::nameRole});
    });
    connect(o,&NftBox::stateChanged,this,[=]{
        const auto ind=idToIndex(o->outId());
        if(ind>-1)
            emit dataChanged(index(ind),index(ind),QList<int>{ModelRoles::stateRole});
    });
    connect(o,&NftBox::uriChanged,this,[=]{
        const auto ind=idToIndex(o->outId());
        if(ind>-1)
            emit dataChanged(index(ind),index(ind),QList<int>{ModelRoles::uriRole});
    });
    beginInsertRows(QModelIndex(), i, i);
    boxes.append(o);
    emit countChanged(count());
    endInsertRows();

}


void BoxModel::rmBox(int i) {
    if(boxes.at(i)->addressBech32().isNull())
    {
        newBoxes_--;
        emit newBoxesChanged();
    }
    boxes[i]->setSelected(false);
    beginRemoveRows(QModelIndex(),i,i);
    boxes[i]->deleteLater();
    boxes.remove(i);
    emit countChanged(count());
    endRemoveRows();
}
int BoxModel::idToIndex(c_array outId)
{
    for(auto i=0;i<boxes.size();i++)
    {
        if(boxes[i]->outId()==outId)
        {
            return i;
        }
    }
    return -1;
}


