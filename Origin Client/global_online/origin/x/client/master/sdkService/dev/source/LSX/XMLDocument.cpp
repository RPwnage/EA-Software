///////////////////////////////////////////////////////////////////////////////
//
//  Copyright © 2008 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: XMLDocument.cpp
//	generic XML wrapper class implementation
//
//	Author: Sergey Zavadski

#include "services/debug/DebugService.h"
#include "LSX/XMLDocument.h"

#include <QBuffer>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QDomNodeList>
#include <QFile>
#include <QTextStream>
#include <QtXmlPatterns/QXmlQuery>

#include <QtXmlPatterns/QAbstractXmlNodeModel>
#include <QtXmlPatterns/QXmlNamePool>
#include <QUrl>
#include <QVector>
#include <QtXmlPatterns/QXmlResultItems>
#include <QtXmlPatterns/QSourceLocation>
#include <QVariant>

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            //////////////////////////////////////////////////////////////////////////
            // Convenience class to handle stack-based QFile opening & closing.

            class QFileAutoCloser
            {
            public:

                QFileAutoCloser(QString const& filePath, QIODevice::OpenMode openFlags)
                    : mFile(filePath)
                    , mIsOpen(false)
                {
                    mIsOpen = mFile.open(openFlags);
                }

                ~QFileAutoCloser()
                {
                    if (mIsOpen)
                    {
                        mFile.close();
                    }
                }

                QFile& File()
                {
                    return mFile;
                }

                bool IsOpen() const
                {
                    return mIsOpen;
                }

            private:

                QFile mFile;
                bool mIsOpen;
            };

            //////////////////////////////////////////////////////////////////////////
            // Implementation of QAbstractXmlNodeModel to allow QXmlQuery to be used
            // on QDomDocuments.

            class QXmlNodeModelForDom: public QAbstractXmlNodeModel
            {
            public:

                typedef QVector<QDomNode> Path;

                QXmlNodeModelForDom(QXmlNamePool, QDomDocument);

                // QAbstractXmlNodeModel interface
                QUrl baseUri( QXmlNodeModelIndex const& n) const;
                QXmlNodeModelIndex::DocumentOrder compareOrder(QXmlNodeModelIndex const& ni1, QXmlNodeModelIndex const& ni2) const;
                QUrl documentUri(QXmlNodeModelIndex const& n) const;
                QXmlNodeModelIndex elementById(QXmlName const& id) const;
                QXmlNodeModelIndex::NodeKind kind(QXmlNodeModelIndex const& nodeIndex) const;
                QXmlName name(QXmlNodeModelIndex const& nodeIndex) const;
                QVector<QXmlName> namespaceBindings(QXmlNodeModelIndex const& n) const;
                QVector<QXmlNodeModelIndex> nodesByIdref(QXmlName const& idref) const;
                QXmlNodeModelIndex root(QXmlNodeModelIndex const& n) const;
                QSourceLocation	sourceLocation(QXmlNodeModelIndex const& index) const;
                QString	stringValue(QXmlNodeModelIndex const& n) const;
                QVariant typedValue(QXmlNodeModelIndex const& node) const;

                // Index conversions
                QXmlNodeModelIndex fromDomNode (QDomNode const& node) const;
                QDomNode toDomNode(QXmlNodeModelIndex const& node) const;

                Path path(const QDomNode&) const;
                int childIndex(const QDomNode&) const;

            protected:

                QVector<QXmlNodeModelIndex> attributes( QXmlNodeModelIndex const& element) const;
                QXmlNodeModelIndex nextFromSimpleAxis(SimpleAxis axis, QXmlNodeModelIndex const& origin) const;

            private:

                mutable QXmlNamePool mPool;
                mutable QDomDocument mDoc;
            };

            // Class override to open access to QDomNodePrivate externally.
            class DomNodePrivateAccessor: public QDomNode
            {
            public:

                DomNodePrivateAccessor(QDomNode const& other)
                    : QDomNode(other)
                {
                }

                DomNodePrivateAccessor(QDomNodePrivate* impl)
                    : QDomNode(impl)
                {
                }

                QDomNodePrivate* getImpl()
                {
                    return impl;
                }
            };

            QXmlNodeModelForDom::QXmlNodeModelForDom(QXmlNamePool pool, QDomDocument doc)
                : mPool(pool)
                , mDoc(doc)
            {
            }

            QUrl QXmlNodeModelForDom::baseUri (QXmlNodeModelIndex const& /*node*/) const
            {
                ORIGIN_ASSERT_MESSAGE(false, "Unimplemented.");
                return QUrl();
            }

            QXmlNodeModelIndex::DocumentOrder QXmlNodeModelForDom::compareOrder (
                QXmlNodeModelIndex const& from,
                QXmlNodeModelIndex const& to) const
            {
                QDomNode n1 = toDomNode(from);
                QDomNode n2 = toDomNode(to);

                if (n1 == n2)
                    return QXmlNodeModelIndex::Is;

                Path p1 = path(n1);
                Path p2 = path(n2);

                for (int i = 1; i < p1.size(); i++)
                {
                    if (p1[i] == n2)
                        return QXmlNodeModelIndex::Follows;
                }

                for (int i = 1; i < p2.size(); i++)
                {
                    if (p2[i] == n1)
                        return QXmlNodeModelIndex::Precedes;
                }

                for (int i = 1; i < p1.size(); i++)
                {
                    for (int j = 1; j < p2.size(); j++)
                    {
                        if (p1[i] == p2[j]) // Common ancestor
                        {
                            int ci1 = childIndex(p1[i-1]);
                            int ci2 = childIndex(p2[j-1]);

                            return (ci1 < ci2) ? QXmlNodeModelIndex::Precedes : QXmlNodeModelIndex::Follows;
                        }
                    }
                }

                return QXmlNodeModelIndex::Precedes; // Should be impossible!
            }

            QUrl QXmlNodeModelForDom::documentUri (QXmlNodeModelIndex const& /*node*/) const
            {
                ORIGIN_ASSERT_MESSAGE(false, "Unimplemented.");
                return QUrl();
            }

            QXmlNodeModelIndex QXmlNodeModelForDom::elementById(QXmlName const& id) const
            {
                return fromDomNode(mDoc.elementById(id.toClarkName(mPool)));
            }

            QXmlNodeModelIndex::NodeKind QXmlNodeModelForDom::kind(QXmlNodeModelIndex const& nodeIndex) const
            {
                switch (toDomNode(nodeIndex).nodeType())
                {
                case QDomNode::AttributeNode:
                    return QXmlNodeModelIndex::Attribute; break;

                case QDomNode::CommentNode:
                    return QXmlNodeModelIndex::Comment; break;

                case QDomNode::DocumentNode:
                    return QXmlNodeModelIndex::Document; break;

                case QDomNode::ElementNode:
                    return QXmlNodeModelIndex::Element; break;

                case QDomNode::ProcessingInstructionNode:
                    return QXmlNodeModelIndex::ProcessingInstruction; break;

                case QDomNode::TextNode:
                    return QXmlNodeModelIndex::Text; break;

                default:
                    break;
                }

                return static_cast<QXmlNodeModelIndex::NodeKind>(0);
            }

            QXmlName QXmlNodeModelForDom::name(QXmlNodeModelIndex const& nodeIndex) const
            {
                QDomNode n = toDomNode(nodeIndex);

                if (n.isAttr() || n.isElement() || n.isProcessingInstruction())
                    return QXmlName(mPool, n.nodeName(), QString(), QString());

                return QXmlName(mPool, QString(), QString(), QString());
            }

            QVector<QXmlName> QXmlNodeModelForDom::namespaceBindings(QXmlNodeModelIndex const& /*nodeIndex*/) const
            {
                //ORIGIN_ASSERT_MESSAGE(false, "Unimplemented.");
                return QVector<QXmlName>();
            }

            QVector<QXmlNodeModelIndex> QXmlNodeModelForDom::nodesByIdref(QXmlName const& /*nodeIndex*/) const
            {
                ORIGIN_ASSERT_MESSAGE(false, "Unimplemented.");
                return QVector<QXmlNodeModelIndex>();
            }

            QXmlNodeModelIndex QXmlNodeModelForDom::root(QXmlNodeModelIndex const& nodeIndex) const
            {
                QDomNode node;

                for (node = toDomNode(nodeIndex); !node.parentNode().isNull(); node = node.parentNode())
                    ;

                return fromDomNode(node);
            }

            QSourceLocation	QXmlNodeModelForDom::sourceLocation(QXmlNodeModelIndex const& /*nodeIndex*/) const
            {
                ORIGIN_ASSERT_MESSAGE(false, "Unimplemented.");
                return QSourceLocation();
            }

            QString	QXmlNodeModelForDom::stringValue(QXmlNodeModelIndex const& nodeIndex) const
            {
                QDomNode node = toDomNode(nodeIndex);

                switch (toDomNode(nodeIndex).nodeType())
                {
                case QDomNode::AttributeNode:
                    return node.toAttr().value();

                case QDomNode::CommentNode:
                    return node.toComment().data();

                case QDomNode::DocumentNode:
                    return node.toDocument().documentElement().text();

                case QDomNode::ElementNode:
                    return node.toElement().text();

                case QDomNode::ProcessingInstructionNode:
                    return node.toProcessingInstruction().data();

                case QDomNode::TextNode:
                    return node.toText().nodeValue();

                default:
                    break;
                }

                return QString();
            }

            QVariant QXmlNodeModelForDom::typedValue(QXmlNodeModelIndex const& nodeIndex) const
            {
                return qVariantFromValue(stringValue(nodeIndex));
            }

            QXmlNodeModelIndex QXmlNodeModelForDom::fromDomNode(QDomNode const& node) const
            {
                if (node.isNull())
                    return QXmlNodeModelIndex();

                return createIndex(DomNodePrivateAccessor(node).getImpl(), 0);
            }

            QDomNode QXmlNodeModelForDom::toDomNode(QXmlNodeModelIndex const& nodeIndex) const
            {
                return DomNodePrivateAccessor((QDomNodePrivate*) nodeIndex.data());
            }

            QXmlNodeModelForDom::Path QXmlNodeModelForDom::path(QDomNode const& node) const
            {
                Path res;

                for (QDomNode cur = node; !cur.isNull(); cur = cur.parentNode())
                {
                    res.push_back(cur);
                }
                return res;
            }

            int QXmlNodeModelForDom::childIndex(QDomNode const& node) const
            {
                QDomNodeList children = node.parentNode().childNodes();
                uint numChildren = children.count();
                for (uint i = 0; i < numChildren; ++i)
                {
                    if (children.at(i) == node)
                        return i;
                }

                return -1;
            }

            QVector<QXmlNodeModelIndex> QXmlNodeModelForDom::attributes(QXmlNodeModelIndex const& nodeIndex) const
            {
                QVector<QXmlNodeModelIndex> res;

                QDomElement element = toDomNode(nodeIndex).toElement();
                QDomNamedNodeMap attrs = element.attributes();
                uint numAttrs = attrs.count();
                for (uint i = 0; i < numAttrs; ++i)
                {
                    res.push_back(fromDomNode(attrs.item(i)));
                }

                return res;
            }

            QXmlNodeModelIndex QXmlNodeModelForDom::nextFromSimpleAxis(SimpleAxis axis, QXmlNodeModelIndex const& nodeIndex) const
            {
                QDomNode node = toDomNode(nodeIndex);

                switch(axis)
                {
                case Parent:
                    return fromDomNode(node.parentNode());

                case FirstChild:
                    return fromDomNode(node.firstChild());

                case PreviousSibling:
                    return fromDomNode(node.previousSibling());

                case NextSibling:
                    return fromDomNode(node.nextSibling());
                }

                return QXmlNodeModelIndex();
            }

            //////////////////////////////////////////////////////////////////////////
            // Implementations

            bool LoadXmlFromFile(QDomDocument& documentToLoad, QString const& filePath)
            {
                QFileAutoCloser file(filePath, QIODevice::ReadOnly | QIODevice::Text);
                if (!file.IsOpen())
                    return false;

                if (!documentToLoad.setContent(&file.File()))
                    return false;

                return true;
            }

            bool SaveXmlToFile(QDomDocument& documentToSave, QString const& filePath)
            {
                QFileAutoCloser file(filePath, QIODevice::WriteOnly | QIODevice::Text);
                if (!file.IsOpen())
                    return false;

                QTextStream stream(&file.File());
                documentToSave.save( stream, 2 );

                return true;
            }

        }

    }
}