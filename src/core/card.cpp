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

#include "card.h"
#include "eventtype.h"
#include "gamelogic.h"
#include "serverplayer.h"

Card::Card(Suit suit, int number)
    : m_suit(suit)
    , m_number(number)
    , m_color(NoColor)
    , m_transferable(false)
    , m_willThrow(true)
    , m_canRecast(false)
    , m_targetFixed(false)
{
    static uint cardId = 0;
    if (m_suit != NoSuit && 1 <= number && number <= 13) {
        cardId++;
        m_id = cardId;
    } else {
        m_id = 0;
    }
}

Card *Card::clone() const
{
    const QMetaObject *metaObject = this->metaObject();
    Card *card = qobject_cast<Card *>(metaObject->newInstance(Q_ARG(Suit, suit()), Q_ARG(int, number())));
    card->m_id = m_id;
    return card;
}

uint Card::effectiveId() const
{
    if (!isVirtual())
        return m_id;

    if (m_subcards.length() == 1)
        return m_subcards.first()->effectiveId();

    return 0;
}

Card::Suit Card::suit() const
{
    if (m_subcards.isEmpty())
        return m_suit;
    else if (m_subcards.length() == 1)
        return m_subcards.first()->suit();
    else
        return NoSuit;
}

void Card::setSuitString(const QString &suit)
{
    if (suit == "spade")
        setSuit(Spade);
    else if (suit == "heart")
        setSuit(Heart);
    else if (suit == "club")
        setSuit(Club);
    else if (suit == "diamond")
        setSuit(Diamond);
    else
        setSuit(NoSuit);
}

QString Card::suitString() const
{
    if (m_suit == Spade)
        return "spade";
    else if (m_suit == Heart)
        return "heart";
    else if (m_suit == Club)
        return "club";
    else if (m_suit == Diamond)
        return "diamond";
    else
        return "no_suit";
}

int Card::number() const
{
    if (m_number > 0)
        return m_number;

    int number = 0;
    foreach (const Card *card, m_subcards)
        number += card->number();
    return number >= 13 ? 13 : number;
}

Card::Color Card::color() const
{
    if (m_suit == NoSuit)
        return m_color;
    return (m_suit == Spade || m_suit == Club) ? Black : Red;
}

void Card::setColorString(const QString &color)
{
    if (color == "black")
        setColor(Black);
    else if (color == "red")
        setColor(Red);
    else
        setColor(NoColor);
}

QString Card::colorString() const
{
    if (m_color == Black)
        return "black";
    else if (m_color == Red)
        return "red";
    else
        return "no_color";
}

QString Card::typeString() const
{
    if (m_type == BasicType)
        return "basic";
    else if (m_type == TrickType)
        return "trick";
    else if (m_type == EquipType)
        return "equip";
    else
        return "skill";
}

void Card::addSubcard(Card *card)
{
    m_subcards << card;
}

Card *Card::realCard()
{
    if (id() > 0)
        return this;

    if (m_subcards.length() == 1)
        return m_subcards.first()->realCard();

    return nullptr;
}

const Card *Card::realCard() const
{
    if (id() > 0)
        return this;

    if (m_subcards.length() == 1)
        return m_subcards.first()->realCard();

    return nullptr;
}

QList<Card *> Card::realCards()
{
    QList<Card *> cards;
    if (id() > 0) {
        cards << this;
    } else {
        foreach (Card *card, m_subcards)
            cards << card->realCards();
    }
    return cards;
}

QList<const Card *> Card::realCards() const
{
    QList<const Card *> cards;
    if (id() > 0) {
        cards << this;
    } else {
        foreach (const Card *card, m_subcards)
            cards << card->realCards();
    }
    return cards;
}

bool Card::targetFeasible(const QList<const Player *> &targets, const Player *self) const
{
    Q_UNUSED(self)

    if (isTargetFixed())
        return true;
    return !targets.isEmpty();
}

bool Card::targetFilter(const QList<const Player *> &targets, const Player *toSelect, const Player *self) const
{
    return targets.isEmpty() && toSelect != self;
}

bool Card::isAvailable(const Player *player) const
{
    //@todo:
    Q_UNUSED(player)
    return false;
}

void Card::onUse(GameLogic *logic, CardUseStruct &use)
{
    logic->sortByActionOrder(use.to);

    QVariant useData = QVariant::fromValue(&use);
    logic->trigger(PreCardUsed, use.from, useData);

    CardsMoveStruct move;
    move.to.type = CardArea::Table;
    move.isOpen = true;
    move.cards = use.card->realCards();
    logic->moveCards(move);
}

void Card::use(GameLogic *logic, ServerPlayer *source, QList<ServerPlayer *> &targets)
{
    foreach (ServerPlayer *target, targets) {
        CardEffectStruct effect;
        effect.card = this;
        effect.from = source;
        effect.to = target;
        effect.multiple = (targets.length() > 1);
        //@to-do: effect.nullified = ?
        logic->takeCardEffect(effect);
    }

    const CardArea *table = logic->table();
    if (table->length() > 0) {
        CardsMoveStruct move;
        move.cards = table->cards();
        move.to.type = CardArea::DiscardPile;
        move.isOpen = true;
        logic->moveCards(move);
    }
}

void Card::onEffect(GameLogic*, CardEffectStruct &)
{
}

bool Card::isCancelable(const CardEffectStruct &) const
{
    return false;
}

void Card::onNullified(ServerPlayer *) const
{
}

BasicCard::BasicCard(Card::Suit suit, int number)
    : Card(suit, number)
{
    m_type = BasicType;
}


TrickCard::TrickCard(Card::Suit suit, int number)
    : Card(suit, number)
    , m_cancelable(true)
{
    m_type = TrickType;
}

bool TrickCard::isCancelable(const CardEffectStruct &) const
{
    return m_cancelable;
}

EquipCard::EquipCard(Card::Suit suit, int number, Skill *skill)
    : Card(suit, number)
    , m_skill(skill)
{
    m_type = EquipType;
}

void EquipCard::onUse(GameLogic *logic, CardUseStruct &use)
{
    ServerPlayer *player = use.from;
    if (use.to.isEmpty())
        use.to << player;

    QVariant data = QVariant::fromValue(&use);
    logic->trigger(PreCardUsed, player, data);
}

void EquipCard::use(GameLogic *logic, ServerPlayer *, QList<ServerPlayer *> &targets)
{
    if (targets.isEmpty()) {
        CardsMoveStruct move;
        move.cards << this;
        move.to.type = CardArea::DiscardPile;
        move.isOpen = true;
        logic->moveCards(move);
        return;
    }

    ServerPlayer *target = targets.first();

    //Find the existing equip
    Card *equippedCard = nullptr;
    QList<Card *> equips = target->equips()->cards();
    foreach (Card *card, equips) {
        if (card->subtype() == subtype()) {
            equippedCard = card;
            break;
        }
    }

    QList<CardsMoveStruct> moves;

    CardsMoveStruct install;
    install.cards << this;
    install.to.type = CardArea::Equip;
    install.to.owner = target;
    install.isOpen = true;
    moves << install;

    if (equippedCard != nullptr) {
        CardsMoveStruct uninstall;
        uninstall.cards << equippedCard;
        uninstall.to.type = CardArea::Table;
        uninstall.isOpen = true;
        moves << uninstall;
    }
    logic->moveCards(moves);

    if (equippedCard != nullptr) {
        const CardArea *table = logic->table();
        if (table->contains(equippedCard)) {
            CardsMoveStruct discard;
            discard.cards << equippedCard;
            discard.to.type = CardArea::DiscardPile;
            discard.isOpen = true;
            logic->moveCards(discard);
        }
    }
}

GlobalEffect::GlobalEffect(Card::Suit suit, int number)
    : TrickCard(suit, number)
{
    m_targetFixed = true;
    m_subtype = GlobalEffectType;
}

void GlobalEffect::onUse(GameLogic *logic, CardUseStruct &use)
{
    if (use.to.isEmpty())
        use.to = logic->otherPlayers(use.from);
    TrickCard::onUse(logic, use);
}

AreaOfEffect::AreaOfEffect(Card::Suit suit, int number)
    : TrickCard(suit, number)
{
    m_targetFixed = true;
    m_subtype = AreaOfEffectType;
}

void AreaOfEffect::onUse(GameLogic *logic, CardUseStruct &use)
{
    if (use.to.isEmpty())
        use.to = logic->otherPlayers(use.from);
    TrickCard::onUse(logic, use);
}

SingleTargetTrick::SingleTargetTrick(Card::Suit suit, int number)
    : TrickCard(suit, number)
{
    m_subtype = SingleTargetType;
}

bool SingleTargetTrick::targetFilter(const QList<const Player *> &, const Player *, const Player *) const
{
    return true;
}


DelayedTrick::DelayedTrick(Card::Suit suit, int number)
    : TrickCard(suit, number)
{
    m_subtype = DelayedType;
}

void DelayedTrick::onUse(GameLogic *logic, CardUseStruct &use)
{
    use.card = this;

    QVariant data = QVariant::fromValue(&use);
    logic->trigger(PreCardUsed, use.from, data);

    CardsMoveStruct move;
    move.cards = use.card->realCards();
    move.to.type = CardArea::DelayedTrick;
    move.to.owner = use.to.first();
    move.isOpen = true;
    logic->moveCards(move);
}
