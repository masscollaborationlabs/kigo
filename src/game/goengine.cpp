/*******************************************************************
 *
 * Copyright 2008 Sascha Peilicke <sasch.pe@gmx.de>
 *
 * This file is part of the KDE project "KGo"
 *
 * KGo is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * KGo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KReversi; see the file COPYING.  If not, write to
 * the Free Software Foundation, 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 *******************************************************************/

/**
 * @file This file is part of KGO and implements the classes GoEngine, GoEngine::Stone
 *       and GoEngine::Score, which together implement a Go Text Protocol (GTP)
 *       interface to communicate with Go engines supporting GTP protocol
 *       version 2.
 *
 * @author Sascha Peilicke <sasch.pe@gmx.de>
 */
#include "goengine.h"

#include <KDebug>

#include <QFile>
#include <QApplication>

namespace KGo {

GoEngine::Stone::Stone(const QString &stone)
    : m_x(stone[0].toLatin1())
    , m_y(stone.mid(1).toInt())
{
}

bool GoEngine::Stone::isValid() const
{
    return (m_y >= 1 && m_y <= 19 && ((m_x >= 'a' && m_x < 't') || (m_x >= 'A' && m_x <= 'T')));
}

QByteArray GoEngine::Stone::toLatin1() const
{
    QByteArray msg;
    msg.append(m_x);
    msg.append(m_y);
    return msg;
}

QString GoEngine::Stone::toString() const
{
    return QString("%1%2").arg(m_x).arg(m_y);
}

GoEngine::Score::Score(const QString &scoreString)
{
    if (scoreString[0] == 'W')
        m_player = WhitePlayer;
    else
        m_player = BlackPlayer;
    int i = scoreString.indexOf(' ');
    m_score = scoreString.mid(2, i - 1).toInt();
    //m_upperBound = scoreString.mid(
    //TODO: Implement Score class, the bounds should be considered optional and can
    // be the same as the score if none given
}

bool GoEngine::Score::isValid() const
{
    return m_score >= 0;
}

QString GoEngine::Score::toString() const
{
    return QString("%1+%2 (upper bound: %3, lower: %4").arg(m_player == WhitePlayer ? 'W' : 'B').arg(m_score).arg(m_upperBound).arg(m_lowerBound);
}

////////////////////////////////////////////////////////////////////

GoEngine::GoEngine()
{
    connect(&m_process, SIGNAL(readyReadStandardError()), this, SLOT(readStandardError()));
    connect(&m_process, SIGNAL(error(QProcess::ProcessError)), SIGNAL(error(QProcess::ProcessError)));
}

GoEngine::~GoEngine()
{
    quit();
}

bool GoEngine::run(const QString &command)
{
    quit();                                         // Close old session if there's one
    m_process.start(command.toLatin1());            // Start new process with provided command

    if (!m_process.waitForStarted())                // NOTE: Blocking wait for process start
        return false;

    kDebug() << "Run new GTP engine session";

    if (protocolVersion() == 2) {                    // We support only GTP version 2 for now
        clearBoard();                               // Start with blank board
    } else {
        kDebug() << "Protocol version error:" << protocolVersion();
        quit();
        m_response = "Protocol version error";
        return false;
    }
    return true;
}

void GoEngine::quit()
{
    if (m_process.isOpen()) {
        kDebug() << "Quit GTP engine session";
        m_process.write("quit\n");
        m_process.close();
    }
}

bool GoEngine::loadSgf(const QString &fileName, int moveNumber)
{
    Q_ASSERT(moveNumber >= 0);
    if (fileName.isEmpty() || !QFile::exists(fileName))
        return false;

    kDebug() << "Attempting to load move" << moveNumber << "from" << fileName;

    QByteArray msg("loadsgf " + fileName.toLatin1() + ' ');
    msg.append(QByteArray::number(moveNumber));
    msg.append('\n');
    m_process.write(msg);
    if (waitResponse()) {
        emit boardChanged();
        return true;
    } else
        return false;
}

bool GoEngine::saveSgf(const QString &fileName)
{
    if (fileName.isEmpty())
        return false;

    kDebug() << "Attempting to save game to" << fileName;

    m_process.write("printsgf " + fileName.toLatin1() + '\n');
    return waitResponse();
}

QString GoEngine::name()
{
    m_process.write("name\n");
    return waitResponse() ? m_response : QString();
}

int GoEngine::protocolVersion()
{
    m_process.write("protocol_version\n");
    return waitResponse() ? m_response.toInt() : -1;
}

QString GoEngine::version()
{
    m_process.write("version\n");
    return waitResponse() ? m_response : QString();
}

bool GoEngine::setBoardSize(int size)
{
    Q_ASSERT(size >= 1 && size <= 19);
    kDebug() << "Set board size to"  << size;

    QByteArray msg("boardsize ");
    msg.append(QByteArray::number(size));
    msg.append('\n');
    m_process.write(msg);
    if (waitResponse()) {
        emit boardChanged();
        emit boardSizeChanged(size);
        return true;
    } else
        return false;
}

int GoEngine::boardSize()
{
    m_process.write("query_boardsize\n");
    return waitResponse() ? m_response.toInt() : -1;
}

bool GoEngine::clearBoard()
{
    kDebug() << "Clear board";

    m_process.write("clear_board\n");
    if (waitResponse()) {
        emit boardChanged();
        return true;
    } else
        return false;
}

bool GoEngine::setKomi(float komi)
{
    Q_ASSERT(komi >= 0);
    kDebug() << "Set komi to" << komi;

    QByteArray msg("komi ");
    msg.append(QByteArray::number(komi));
    msg.append('\n');
    m_process.write(msg);
    return waitResponse();
}

bool GoEngine::setLevel(int level)
{
    Q_ASSERT(level >= 1 && level <= 10);
    kDebug() << "Set difficulty level to" << level;

    QByteArray msg("level ");
    msg.append(QByteArray::number(level));
    msg.append('\n');
    m_process.write(msg);
    return waitResponse();
}

bool GoEngine::setFixedHandicap(int handicap)
{
    Q_ASSERT(handicap >= 0 && handicap <= 9);
    kDebug() << "Set fixed handicap to" << handicap;

    QByteArray msg("fixed_handicap ");
    msg.append(QByteArray::number(handicap));
    msg.append('\n');
    m_process.write(msg);
    if (waitResponse()) {
        emit boardChanged();
        return true;
    } else
        return false;
}

bool GoEngine::playMove(PlayerColor color, const Stone &field)
{
    if (!field.isValid() || color == InvalidPlayer)
        return false;

    QByteArray msg("play ");
    if (color == WhitePlayer)
        msg.append("white ");
    else
        msg.append("black ");
    msg.append(field.toLatin1() + '\n');
    m_process.write(msg);
    if (waitResponse()) {
        emit boardChanged();
        if (color == WhitePlayer)
            emit nextTurn(BlackPlayer);
        else
            emit nextTurn(WhitePlayer);
        return true;
    } else
        return false;
}

bool GoEngine::passMove(PlayerColor color)
{
    if (!color == WhitePlayer || color == BlackPlayer)
        return false;

    QByteArray msg("play ");
    if (color == WhitePlayer)
        msg.append("white ");
    else
        msg.append("black ");
    msg.append("pass\n");
    m_process.write(msg);
    if (waitResponse()) {
        emit boardChanged();
        if (color == WhitePlayer)
            emit nextTurn(BlackPlayer);
        else
            emit nextTurn(WhitePlayer);
        return true;
    } else
        return false;
}

bool GoEngine::generateMove(PlayerColor color)
{
    if (!color == WhitePlayer || color == BlackPlayer)
        return false;

    QByteArray msg("genmove ");
    if(color == WhitePlayer)
        msg.append("white\n");
    else
        msg.append("black\n");
    m_process.write(msg);
    if (waitResponse()) {
        emit boardChanged();
        if (color == WhitePlayer)
            emit nextTurn(BlackPlayer);
        else
            emit nextTurn(WhitePlayer);
        return true;
    } else
        return false;
}

bool GoEngine::undoMove(int i)
{
    Q_ASSERT(i >= 0);
    QByteArray msg("undo ");
    msg.append(QByteArray::number(i));
    msg.append('\n');
    m_process.write(msg);
    if (waitResponse()) {
        emit boardChanged();
        m_process.write("last_move\n");
        if (waitResponse()) {
            if (m_response.startsWith("white"))
                emit nextTurn(WhitePlayer);
            else if (m_response.startsWith("black"))
                emit nextTurn(BlackPlayer);
            else
                return false;
        } else
            return false;
        return true;
    } else
        return false;
}

bool GoEngine::tryMove(PlayerColor color, const Stone &field)
{
    if (!field.isValid() || color == InvalidPlayer)
        return false;

    QByteArray msg("trymove ");
    if (color == WhitePlayer)
        msg.append("white ");
    else
        msg.append("black ");
    msg.append(field.toLatin1() + '\n');
    m_process.write(msg);
    if (waitResponse()) {
        emit boardChanged();
        if (color == WhitePlayer)
            emit nextTurn(BlackPlayer);
        else
            emit nextTurn(WhitePlayer);
        return true;
    } else
        return false;
}

bool GoEngine::popGo()
{
    m_process.write("popgo\n");
    if (waitResponse()) {
        emit boardChanged();
        return true;
    } else
        return false;
}

QPair<GoEngine::PlayerColor, GoEngine::Stone> GoEngine::lastMove()
{
    m_process.write("last_move\n");
    QPair<PlayerColor, Stone> pair(InvalidPlayer, Stone());
    if (waitResponse()) {
        if (m_response.startsWith("white")) {
            pair.first = WhitePlayer;
            pair.second = Stone(m_response.split(' ')[1]);
        } else if (m_response.startsWith("black")) {
            pair.first = BlackPlayer;
            pair.second = Stone(m_response.split(' ')[1]);
        }
    }
    return pair;
}

GoEngine::FieldStatus GoEngine::whatColor(const Stone &field)
{
    if (!field.isValid())
        return InvalidField;

    m_process.write("color " + field.toLatin1() + '\n');
    if (waitResponse()) {
        if (m_response == "white") return WhiteField;
        else if (m_response == "black") return BlackField;
        else if (m_response == "empty") return EmptyField;
        else return InvalidField;
    } else
        return InvalidField;
}

QList<GoEngine::Stone> GoEngine::listStones(PlayerColor color)
{
    QList<Stone> list;
    if (color == InvalidPlayer)
        return list;

    QByteArray msg("list_stones ");
    if (color == WhitePlayer)
        msg.append("white\n");
    else
        msg.append("black\n");
    m_process.write(msg);
    if (waitResponse()) {
        foreach (const QString &pos, m_response.split(' '))
            list.append(Stone(pos));
    }
    return list;
}

QList<QPair<GoEngine::PlayerColor, GoEngine::Stone> > GoEngine::moveHistory()
{
    QList<QPair<PlayerColor, Stone> > list;

    m_process.write("move_history\n");
    if (waitResponse()) {
        QStringList tokens = m_response.split(' ');
        for (int i = 0; i < tokens.size(); i += 2) {
            if (tokens[i] == "white")
                list.append(QPair<PlayerColor, Stone>(WhitePlayer, Stone(tokens[i + 1])));
            else
                list.append(QPair<PlayerColor, Stone>(BlackPlayer, Stone(tokens[i + 1])));
        }
    }
    return list;
}

QList<GoEngine::Stone> GoEngine::moveHistory(PlayerColor color)
{
    QList<Stone> list;
    if (color == InvalidPlayer)
        return list;

    m_process.write("move_history\n");
    if (waitResponse()) {
        QStringList tokens = m_response.split(' ');
        for (int i = 0; i < tokens.size(); i += 2)
            if (color == WhitePlayer && tokens[i] == "white" || color == BlackPlayer && tokens[i] == "black")
                list.append(Stone(Stone(tokens[i + 1])));
    }
    return list;
}

int GoEngine::countLiberties(const Stone &field)
{
    if (!field.isValid())
        return -1;

    m_process.write("countlib " + field.toLatin1() + '\n');
    return waitResponse() ? m_response.toInt() : -1;
}

QList<GoEngine::Stone> GoEngine::findLiberties(const Stone &field)
{
    QList<GoEngine::Stone> list;
    if (!field.isValid())
        return list;

    m_process.write("findlib " + field.toLatin1() + '\n');
    if (waitResponse()) {
        foreach (const QString &entry, m_response.split(' '))
            list.append(GoEngine::Stone(entry));
    }
    return list;
}

bool GoEngine::isLegal(PlayerColor color, const Stone &field)
{
    if (!field.isValid() || color == InvalidPlayer)
        return false;

    QByteArray msg("is_legal ");
    if (color == WhitePlayer)
        msg.append("white ");
    else
        msg.append("black ");
    msg.append(field.toLatin1() + '\n');
    m_process.write(msg);
    return waitResponse() && m_response == "1";
}

QString GoEngine::topMoves(PlayerColor color)
{
    if (color == InvalidPlayer)
        return QString();

    QByteArray msg("top_moves_");
    if (color == WhitePlayer)
        msg.append("white\n");
    else
        msg.append("black\n");
    m_process.write(msg);
    return waitResponse() ? m_response : QString();
}

QList<GoEngine::Stone> GoEngine::legalMoves(PlayerColor color)
{
    QList<GoEngine::Stone> list;
    if (color == InvalidPlayer)
        return list;

    QByteArray msg("all_legal ");
    if (color == WhitePlayer)
        msg.append("white\n");
    else
        msg.append("black\n");
    m_process.write(msg);
    if (waitResponse()) {
        foreach (const QString &entry, m_response.split(' '))
            list.append(GoEngine::Stone(entry));
    }
    return list;
}

int GoEngine::captures(PlayerColor color)
{
    if (color == InvalidPlayer)
        return 0;

    QByteArray msg("captures ");
    if (color == WhitePlayer)
        msg.append("white\n");
    else
        msg.append("black\n");
    m_process.write(msg);
    return waitResponse() ? m_response.toInt() : 0;
}

QString GoEngine::attack(const Stone &field)
{
    if (!field.isValid())
        return QString();

    m_process.write("attack " + field.toLatin1() + '\n');
    return waitResponse() ? m_response : QString();
}

QString GoEngine::defend(const Stone &field)
{
    if (!field.isValid())
        return QString();

    m_process.write("defend " + field.toLatin1() + '\n');
    return waitResponse() ? m_response : QString();
}

bool GoEngine::increaseDepths()
{
    m_process.write("increase_depths\n");
    return waitResponse();
}

bool GoEngine::decreaseDepths()
{
    m_process.write("decrease_depths\n");
    return waitResponse();
}

QString GoEngine::owlAttack(const Stone &field)
{
    if (!field.isValid())
        return QString();

    m_process.write("owl_attack " + field.toLatin1() + '\n');
    return waitResponse() ? m_response : QString();
}

QString GoEngine::owlDefense(const Stone &field)
{
    if (!field.isValid())
        return QString();

    m_process.write("owl_defend " + field.toLatin1() + '\n');
    return waitResponse() ? m_response : QString();
}

QString GoEngine::evalEye(const Stone &field)
{
    if (!field.isValid())
        return QString();

    m_process.write("eval_eye " + field.toLatin1() + '\n');
    return waitResponse() ? m_response : QString();
}

GoEngine::DragonStatus GoEngine::dragonStatus(const Stone &field)
{
    if (!field.isValid())
        return UnknownDragon;

    m_process.write("dragon_status " + field.toLatin1() + '\n');
    if (waitResponse()) {
        if (m_response == "alive") return AliveDragon;
        else if (m_response == "critical") return CriticalDragon;
        else if (m_response == "dead") return DeadDragon;
        else if (m_response == "unknown") return UnknownDragon;
        else return UnknownDragon;  // Should never happen
    } else
        return UnknownDragon;
}

bool GoEngine::isSameDragon(const Stone &field1, const Stone &field2)
{
    m_process.write("same_dragon " + field1.toLatin1() + ' ' + field2.toLatin1() + '\n');
    return waitResponse() && m_response == "1";
}

QString GoEngine::dragonData(const Stone &field)
{
    QByteArray msg("dragon_data ");
    if (field.isValid())
        msg.append(field.toLatin1());
    msg.append('\n');
    m_process.write(msg);
    return waitResponse() ? m_response : QString();
}

GoEngine::FinalState GoEngine::finalStatus(const Stone &field)
{
    if (!field.isValid())
        return FinalStateInvalid;

    m_process.write("final_status " + field.toLatin1() + '\n');
    if (waitResponse()) {
        if (m_response == "alive") return FinalAlive;
        else if (m_response == "dead") return FinalDead;
        else if (m_response == "seki") return FinalSeki;
        else if (m_response == "white_territory") return FinalWhiteTerritory;
        else if (m_response == "blacK_territory") return FinalBlackTerritory;
        else if (m_response == "dame") return FinalDame;
        else return FinalStateInvalid;
    } else
        return FinalStateInvalid;
}

QList<GoEngine::Stone> GoEngine::finalStatusList(FinalState state)
{
    QList<Stone> list;
    if (state == FinalStateInvalid)
        return list;

    QByteArray msg("final_status_list ");
    switch (state) {
        case FinalAlive: msg.append("alive"); break;
        case FinalDead: msg.append("dead"); break;
        case FinalSeki: msg.append("seki"); break;
        case FinalWhiteTerritory: msg.append("white_territory"); break;
        case FinalBlackTerritory: msg.append("black_territory"); break;
        case FinalDame: msg.append("dame"); break;
        case FinalStateInvalid: /* Will never happen */ break;
    }
    msg.append('\n');
    m_process.write(msg);
    if (waitResponse()) {
        foreach (const QString &entry, m_response.split(' '))
            list.append(Stone(entry));
    }
    return list;
}

GoEngine::Score GoEngine::finalScore()
{
    m_process.write("final_score\n");
    return waitResponse() ? Score(m_response) : Score();
}

GoEngine::Score GoEngine::estimateScore()
{
    m_process.write("estimate_score\n");
    return waitResponse() ? Score(m_response) : Score();
}

int GoEngine::getLifeNodeCounter()
{
    m_process.write("get_life_node_counter\n");
    return waitResponse() ? m_response.toInt() : -1;
}

bool GoEngine::resetLifeNodeCounter()
{
    m_process.write("reset_life_node_counter\n");
    return waitResponse();
}

int GoEngine::getOwlNodeCounter()
{
    m_process.write("get_owl_node_counter\n");
    return waitResponse() ? m_response.toInt() : -1;
}

bool GoEngine::resetOwlNodeCounter()
{
    m_process.write("reset_owl_node_counter\n");
    return waitResponse();
}

int GoEngine::getReadingNodeCounter()
{
    m_process.write("get_reading_node_counter\n");
    return waitResponse() ? m_response.toInt() : -1;
}

bool GoEngine::resetReadingNodeCounter()
{
    m_process.write("reset_reading_node_counter\n");
    return waitResponse();
}

int GoEngine::getTryMoveCounter()
{
    m_process.write("get_trymove_counter\n");
    return waitResponse() ? m_response.toInt() : -1;
}

bool GoEngine::resetTryMoveCounter()
{
    m_process.write("reset_trymove_counter\n");
    return waitResponse();
}

bool GoEngine::showBoard()
{
    m_process.write("showboard\n");
    return waitResponse();
}

bool GoEngine::dumpStack()
{
    m_process.write("dump_stack\n");
    return waitResponse();
}

int GoEngine::wormCutStone(const Stone &field)
{
    if (!field.isValid())
        return -1;

    QByteArray msg("worm_cutstone ");
    msg.append(field.toLatin1());
    msg.append("\n");
    m_process.write(msg);
    return waitResponse() ? m_response.toInt() : -1;
}

QString GoEngine::wormData(const Stone &field)
{
    QByteArray msg("worm_data ");
    if (field.isValid())
        msg.append(field.toLatin1());
    msg.append('\n');
    m_process.write(msg);
    return waitResponse() ? m_response : QString();
}

QList<GoEngine::Stone> GoEngine::wormStones(const Stone &field)
{
    QList<Stone> list;
    if (!field.isValid())
        return list;

    m_process.write("worm_stones " + field.toLatin1() + '\n');
    if (waitResponse()) {
        foreach (const QString &entry, m_response.split(' '))
            list.append(Stone(entry));
    }
    return list;
}

bool GoEngine::tuneMoveOrdering(int parameters)
{
    QByteArray msg("tune_move_ordering ");
    msg.append(QByteArray::number(parameters));
    msg.append('\n');
    m_process.write(msg);
    return waitResponse();
}

QList<QString> GoEngine::help()
{
    m_process.write("help\n");
    QList<QString> list;
    if (waitResponse()) {
        foreach (const QString &entry, m_response.split('\n'))
            list.append(entry);
    }
    return list;
}

bool GoEngine::reportUncertainty(bool enabled)
{
    QByteArray msg("report_uncertainty ");
    msg.append(enabled ? "on" : "off");
    msg.append('\n');
    m_process.write(msg);
    return waitResponse();
}

QString GoEngine::shell(const QString &command)
{
    if (command.isEmpty())
        return QString();

    m_process.write(command.toLatin1() + '\n');
    return waitResponse() ? m_response : QString();
}

bool GoEngine::knownCommand(const QString &command)
{
    if (command.isEmpty())
        return false;

    m_process.write("known_command " + command.toLatin1() + '\n');
    return waitResponse();
}

QString GoEngine::echo(const QString &command)
{
    if (command.isEmpty())
        return QString();

    m_process.write("echo " + command.toLatin1() + '\n');
    return waitResponse() ? m_response : QString();
}

void GoEngine::readStandardError()
{
    kWarning() << "Go engine I/O error occurred:\n" << m_process.readAllStandardError();
}

bool GoEngine::waitResponse()
{
    if (m_process.state() != QProcess::Running) {   // No GTP connection means no computing fun!
        kWarning() << "Go engine command failed because no GTP session is running!";
        return false;
    }

    // Block and wait till command execution finished. The slot GoEngine::readStandardOutput()
    // is invoked when this happens and we continue after the next line.
    m_process.waitForReadyRead();
    m_response = m_process.readAllStandardOutput(); // Reponse arrived, fetch all stdin contents
    //kDebug() << "Returned raw response:" << m_response;

    QChar tmp = m_response[0];                      // First message character indicates success or error
    m_response.remove(0, 2);                        // Remove the first two chars (e.g. "? " or "= ")
    m_response.remove(m_response.size() - 2, 2);    // Remove the two trailing newlines

    emit ready();                                   // Signal that we're done and able to receive next command
    return tmp != '?';                              // '?' Means the engine didn't understand the query
}

} // End of namespace KGo

#include "moc_goengine.cpp"
