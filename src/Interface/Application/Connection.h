/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2015 Scientific Computing and Imaging Institute,
   University of Utah.

   License for the specific language governing rights and limitations under
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#ifndef INTERFACE_APPLICATION_CONNECTION_H
#define INTERFACE_APPLICATION_CONNECTION_H

#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QPen>
#include <boost/function.hpp>
#include <Dataflow/Network/ConnectionId.h>
#include <Interface/Application/Port.h>
#include <Interface/Application/Note.h>
#include <Interface/Application/HasNotes.h>
#include <Core/Utils/Exception.h>

namespace SCIRun {
namespace Gui {

class PortWidget;

class ConnectionDrawStrategy
{
public:
  virtual ~ConnectionDrawStrategy() {}
  virtual void draw(QGraphicsPathItem* item, const QPointF& from, const QPointF& to) = 0;
};

typedef boost::shared_ptr<ConnectionDrawStrategy> ConnectionDrawStrategyPtr;

enum ConnectionDrawType
{
  MANHATTAN, EUCLIDEAN, CUBIC
};

typedef std::pair<SCIRun::Dataflow::Networks::ModuleId, SCIRun::Dataflow::Networks::ModuleId> ModuleIdPair;

class ConnectionLine : public QObject, public QGraphicsPathItem, public HasNotes, public NoteDisplayHelper, public NeedsScenePositionProvider
{
  Q_OBJECT

public:
  ConnectionLine(PortWidget* fromPort, PortWidget* toPort, const SCIRun::Dataflow::Networks::ConnectionId& id, ConnectionDrawStrategyPtr drawer);
  ~ConnectionLine();
  void setColor(const QColor& color);
  void setColorAndWidth(const QColor& color, int width);
  QColor color() const;
  ModuleIdPair getConnectedToModuleIds() const;
  void updateNoteFromFile(const Note& note);
  std::pair<PortWidget*, PortWidget*> connectedPorts() const { return { fromPort_, toPort_ }; }
  const SCIRun::Dataflow::Networks::ConnectionId& id() const { return id_; }
public Q_SLOTS:
  void trackNodes();
  void setDrawStrategy(ConnectionDrawStrategyPtr drawer);
  void updateNote(const Note& note);

Q_SIGNALS:
  void deleted(const SCIRun::Dataflow::Networks::ConnectionId& id);
  void noteChanged();
protected:
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  QVariant itemChange(GraphicsItemChange change, const QVariant& value);
  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void setNoteGraphicsContext() override;
  void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
  void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

private:
  PortWidget* fromPort_;
  PortWidget* toPort_;
  SCIRun::Dataflow::Networks::ConnectionId id_;
  ConnectionDrawStrategyPtr drawer_;
  void destroy();
  bool destroyed_;
  class ConnectionMenu* menu_;
  bool menuOpen_;
  QColor placeHoldingColor_;
  int placeHoldingWidth_;
};

struct InvalidConnection : virtual Core::ExceptionBase {};

class ConnectionInProgress
{
public:
	virtual ~ConnectionInProgress() {}
  virtual void update(const QPointF& end) = 0;
  virtual void makePotential() = 0;
  virtual void highlight(bool on) = 0;
  virtual bool isHighlighted() const = 0;
  virtual QPointF endpoint() const = 0;
  virtual PortWidget* receiver() const = 0;
  virtual void setReceiver(PortWidget* rec) = 0;
};

template <class Base>
class ConnectionInProgressGraphicsItem : public Base, public ConnectionInProgress
{
public:
  ConnectionInProgressGraphicsItem(PortWidget* port, ConnectionDrawStrategyPtr drawer) :
    fromPort_(port), receiver_(0), drawStrategy_(drawer), isHighlighted_(false)
  {
    Base::setZValue(1000); //TODO
    setColor(fromPort_->color());
  }

  void setColor(const QColor& color)
  {
    Base::setPen(QPen(color, 5.0, Qt::DotLine));
  }

  QColor color() const
  {
    return Base::pen().color();
  }

  virtual void makePotential() override
  {
    Base::setOpacity(0.3);
    Base::setPen(QPen(color(), 3.0, Qt::DotLine));
  }

  virtual void highlight(bool on) override
  {
    if (on)
      Base::setPen(QPen(Qt::red, 7.0, Qt::SolidLine));
    else
      Base::setPen(QPen(fromPort_->color(), 3.0, Qt::DotLine));
    isHighlighted_ = on;
  }

  virtual bool isHighlighted() const override { return isHighlighted_; }

  virtual QPointF endpoint() const override
  {
    return lastEnd_;
  }

  virtual PortWidget* receiver() const override
  {
    return receiver_;
  }

  virtual void setReceiver(PortWidget* rec) override
  {
    receiver_ = rec;
  }

protected:
  PortWidget* fromPort_;
  PortWidget* receiver_;
  ConnectionDrawStrategyPtr drawStrategy_;
  QPointF lastEnd_;
  bool isHighlighted_;
};

class ConnectionInProgressStraight : public ConnectionInProgressGraphicsItem<QGraphicsLineItem>
{
public:
  ConnectionInProgressStraight(PortWidget* port, ConnectionDrawStrategyPtr drawer);
  virtual void update(const QPointF& end);
};

class ConnectionInProgressCurved : public ConnectionInProgressGraphicsItem<QGraphicsPathItem>
{
public:
  ConnectionInProgressCurved(PortWidget* port, ConnectionDrawStrategyPtr drawer);
  virtual void update(const QPointF& end);
};

class ConnectionInProgressManhattan : public ConnectionInProgressGraphicsItem<QGraphicsPathItem>
{
public:
  ConnectionInProgressManhattan(PortWidget* port, ConnectionDrawStrategyPtr drawer);
  virtual void update(const QPointF& end);
};

class ConnectionFactory : public QObject
{
  Q_OBJECT
public:
  explicit ConnectionFactory(QGraphicsScene* scene);
  ConnectionInProgress* makeConnectionInProgress(PortWidget* port) const;
  ConnectionInProgress* makePotentialConnection(PortWidget* port) const;
  ConnectionLine* makeFinishedConnection(PortWidget* fromPort, PortWidget* toPort, const SCIRun::Dataflow::Networks::ConnectionId& id) const;
  void setType(ConnectionDrawType type);
  ConnectionDrawType getType() const;
  void setVisibility(bool visible) { visible_ = visible; }
Q_SIGNALS:
  void typeChanged(ConnectionDrawStrategyPtr drawerMaker);
private:
  ConnectionDrawType currentType_;
  bool visible_;
  void activate(QGraphicsItem* item) const;
  QGraphicsScene* scene_;
  ConnectionDrawStrategyPtr euclidean_;
  ConnectionDrawStrategyPtr cubic_;
  ConnectionDrawStrategyPtr manhattan_;
  ConnectionDrawStrategyPtr getCurrentDrawer() const;
};

}
}
#endif
