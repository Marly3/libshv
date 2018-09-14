#pragma once

#include "../shviotqtglobal.h"

//#include <shv/chainpack/rpc.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/core/stringview.h>

#include <QObject>

namespace shv { namespace chainpack { class MetaMethod; class RpcValue; class RpcMessage; class RpcRequest; }}
//namespace shv { namespace core { class StringView; }}

namespace shv {
namespace iotqt {
namespace node {

class ShvRootNode;

class SHVIOTQT_DECL_EXPORT ShvNode : public QObject
{
	Q_OBJECT
public:
	using String = std::string;
	using StringList = std::vector<String>;
	using StringView = shv::core::StringView;
	using StringViewList = shv::core::StringViewList;
public:
	explicit ShvNode(ShvNode *parent = nullptr);

	//size_t childNodeCount() const {return propertyNames().size();}
	ShvNode* parentNode() const;
	virtual ShvNode* childNode(const String &name, bool throw_exc = true) const;
	//ShvNode* childNode(const core::StringView &name) const;
	virtual void setParentNode(ShvNode *parent);
	virtual String nodeId() const {return m_nodeId;}
	void setNodeId(String &&n);
	void setNodeId(const String &n);

	String shvPath() const;
	ShvRootNode* rootNode();

	void deleteDanglingPath();

	virtual bool isRootNode() const {return false;}

	virtual void handleRawRpcRequest(chainpack::RpcValue::MetaData &&meta, std::string &&data);
	virtual void handleRpcRequest(const chainpack::RpcRequest &rq);
	virtual chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq);

	virtual shv::chainpack::RpcValue dir(const StringViewList &shv_path, const shv::chainpack::RpcValue &methods_params);
	//virtual StringList methodNames(const StringViewList &shv_path);

	virtual shv::chainpack::RpcValue ls(const StringViewList &shv_path, const shv::chainpack::RpcValue &methods_params);
	virtual shv::chainpack::RpcValue hasChildren(const StringViewList &shv_path);
	virtual shv::chainpack::RpcValue lsAttributes(const StringViewList &shv_path, unsigned attributes);
public:
	virtual size_t methodCount(const StringViewList &shv_path);
	virtual const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix);
	virtual const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, const std::string &name);

	virtual StringList childNames(const StringViewList &shv_path);
	virtual StringList childNames() {return childNames(StringViewList());}

	//virtual shv::chainpack::RpcValue callMethodRaw(const StringViewList &shv_path, const std::string &method, const std::string &data);
	virtual shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params);
private:
	String m_nodeId;
};

class SHVIOTQT_DECL_EXPORT ShvRootNode : public ShvNode
{
	Q_OBJECT
	using Super = ShvNode;
public:
	explicit ShvRootNode(QObject *parent) : Super() {setParent(parent);}

	bool isRootNode() const override {return true;}

	Q_SIGNAL void sendRpcMesage(const shv::chainpack::RpcMessage &msg);
	void emitSendRpcMesage(const shv::chainpack::RpcMessage &msg);
};

}}}
