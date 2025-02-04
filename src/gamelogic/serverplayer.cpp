/********************************************************************
    Copyright (c) 2013-2015 - Mogara

    This file is part of QSanguosha.

    This game engine is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3.0
    of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    See the LICENSE file for more details.

    Mogara
*********************************************************************/

#include "gamelogic.h"
#include "protocol.h"
#include "serverplayer.h"

#include <croom.h>
#include <cserveragent.h>

ServerPlayer::ServerPlayer(GameLogic *logic, CServerAgent *agent)
    : Player(logic)
    , m_logic(logic)
    , m_room(logic->room())
    , m_agent(agent)
{
}

ServerPlayer::~ServerPlayer()
{
}

CServerAgent *ServerPlayer::agent() const
{
    return m_agent.data();
}

void ServerPlayer::setAgent(CServerAgent *agent)
{
    m_agent = agent;
}

CRoom *ServerPlayer::room() const
{
    if (m_room->isAbandoned())
        throw GameFinish;
    return m_room;
}

void ServerPlayer::drawCards(int n)
{
    CardsMoveStruct move;
    move.from.type = CardArea::DrawPile;
    move.from.direction = CardArea::Top;
    move.to.type = CardArea::Hand;
    move.to.owner = this;
    move.cards = m_logic->drawPile()->first(n);
    m_logic->moveCards(move);
}

void ServerPlayer::play()
{
    QList<Phase> phases;
    phases << RoundStart
           << Start
           << Judge
           << Draw
           << Play
           << Discard
           << Finish;
    play(phases);
}

void ServerPlayer::play(const QList<Player::Phase> &phases)
{
    PhaseChangeStruct change;
    foreach (Phase to, phases) {
        if (to == NotActive)
            break;
        change.from = phase();
        change.to = to;

        QVariant data = QVariant::fromValue(&change);
        bool skip = m_logic->trigger(PhaseChanging, this, data);

        setPhase(change.to);
        broadcastProperty("phase");

        if (skip && !m_logic->trigger(PhaseSkipping, this, data))
            continue;

        if (!m_logic->trigger(PhaseStart, this))
            m_logic->trigger(PhaseProceeding, this);
        m_logic->trigger(PhaseEnd, this);
    }

    change.from = phase();
    change.to = NotActive;

    QVariant data = QVariant::fromValue(&change);
    m_logic->trigger(PhaseChanging, this, data);

    setPhase(change.to);
    broadcastProperty("phase");
}

void ServerPlayer::activate(CardUseStruct &use)
{
    int timeout = 15 * 1000;
    if (m_agent.isNull())
        return;
    m_agent->request(S_COMMAND_USE_CARD, QVariant(), timeout);
    if (m_agent.isNull())
        return;
    QVariant replyData = m_agent->waitForReply(timeout);
    if (replyData.isNull())
        return;
    QVariantMap reply = replyData.toMap();
    if (reply.isEmpty())
        return;

    use.from = this;

    uint cardId = reply["cardId"].toUInt();
    use.card = m_logic->findCard(cardId);

    //@to-do: filter view as skills on server side

    QVariantList tos = reply["to"].toList();
    foreach (const QVariant &to, tos) {
        uint toId = to.toUInt();
        ServerPlayer *target = m_logic->findPlayer(toId);
        if (target)
            use.to << target;
    }
}

Event ServerPlayer::askForTriggerOrder(const QString &reason, QList<Event> &options, bool cancelable)
{
    //@todo:
    C_UNUSED(reason);
    C_UNUSED(options);
    C_UNUSED(cancelable);
    return Event();
}

void ServerPlayer::broadcastProperty(const char *name) const
{
    QVariantList data;
    data << id();
    data << name;
    data << property(name);
    m_room->broadcastNotification(S_COMMAND_UPDATE_PLAYER_PROPERTY, data);
}

void ServerPlayer::broadcastProperty(const char *name, const QVariant &value, ServerPlayer *except) const
{
    QVariantList data;
    data << id();
    data << name;
    data << value;
    m_room->broadcastNotification(S_COMMAND_UPDATE_PLAYER_PROPERTY, data, except->agent());
}

void ServerPlayer::notifyPropertyTo(const char *name, ServerPlayer *player)
{
    QVariantList data;
    data << id();
    data << name;
    data << property(name);
    CServerAgent *agent = player->agent();
    if (agent)
        agent->notify(S_COMMAND_UPDATE_PLAYER_PROPERTY, data);
}

void ServerPlayer::addCardHistory(const QString &name, int times)
{
    Player::addCardHistory(name, times);
    QVariantList data;
    data << name;
    data << times;

    if (!m_agent.isNull())
        m_agent->notify(S_COMMAND_ADD_CARD_HISTORY, data);
}

void ServerPlayer::clearCardHistory()
{
    Player::clearCardHistory();
    if (!m_agent.isNull())
        m_agent->notify(S_COMMAND_ADD_CARD_HISTORY);
}
