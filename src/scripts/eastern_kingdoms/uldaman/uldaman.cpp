/* Copyright (C) 2006 - 2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Uldaman
SD%Complete: 100
SDComment: Quest support: 2278 + 1 trash mob.
SDCategory: Uldaman
EndScriptData */

/* ContentData
mob_jadespine_basilisk
npc_lore_keeper_of_norgannon
EndContentData */

#include "scriptPCH.h"
#include "uldaman.h"

enum eSpells
{
    SPELL_CRYSTALLINE_SLUMBER = 3636,
    SPELL_TRAMPLE = 5568,
    SPELL_SELF_DESTRUCT = 9874,
    SPELL_STONED = 10255,
};

/*######
 ## go_keystone_chamber
 ######*/

bool GOHello_go_keystone_chamber(Player* pPlayer, GameObject* pGo)
{
    ScriptedInstance* pInstance = (ScriptedInstance*)pGo->GetInstanceData();

    if (!pInstance)
        return false;

    pInstance->SetData(DATA_IRONAYA_SEAL, IN_PROGRESS);

    return false;
}

struct mob_stone_keeperAI : public ScriptedAI
{
    mob_stone_keeperAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_creature->CastSpell(m_creature, SPELL_STONED, false);
        Reset();
    }

    uint32 m_uiTrample_Timer;

    void Reset()
    {
        m_uiTrample_Timer = urand(4000, 9000);
    }

    void Aggro(Unit* pWho)
    {
        if (m_creature->HasAura(10255))
            m_creature->RemoveAurasDueToSpell(10255);
    }

    void JustDied(Unit* pWho)
    {
        m_creature->CastSpell(m_creature, SPELL_SELF_DESTRUCT, false);

        /** Unlock another Stone Keeper */
        std::list<Creature*> m_StoneKeeperList;
        GetCreatureListWithEntryInGrid(m_StoneKeeperList, m_creature, NPC_STONE_KEEPER, 30.0f);
        for (std::list<Creature*>::iterator it = m_StoneKeeperList.begin(); it != m_StoneKeeperList.end(); ++it)
        {
            Creature* target;
            if (ScriptedInstance* pInstance = (ScriptedInstance*)m_creature->GetInstanceData())
                target = pInstance->GetCreature((*it)->GetObjectGuid());
            else
                return;

            /** Check if a creature is available to become alive */
            if (!target || !target->isAlive() || target->getFaction() == 14)
                continue;

            /** Creature become alive */
            target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            target->setFaction(14);
            target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
            target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            if (target->HasAura(10255))
                target->RemoveAurasDueToSpell(10255);
            SetCombatMovement(true);
            if (Unit* victim = target->SelectNearestTarget(18.0f))
                target->AI()->AttackStart(victim);
            return; // Only one Creature per action
        }
        m_StoneKeeperList.clear();
        /** Open the Altar's door */
        if (ScriptedInstance* pInstance = (ScriptedInstance*)m_creature->GetInstanceData())
            pInstance->SetData(DATA_ALTAR_DOORS, DONE);
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiTrample_Timer < diff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_TRAMPLE) == CAST_OK)
                m_uiTrample_Timer = urand(4000, 10000);
        }
        else m_uiTrample_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mob_stone_keeper(Creature* pCreature)
{
    return new mob_stone_keeperAI(pCreature);
}


/*######
## mob_jadespine_basilisk
######*/

struct mob_jadespine_basiliskAI : public ScriptedAI
{
    mob_jadespine_basiliskAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 Cslumber_Timer;

    void Reset()
    {
        Cslumber_Timer = 2000;
    }

    void UpdateAI(const uint32 diff)
    {
        //Return since we have no target
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        //Cslumber_Timer
        if (Cslumber_Timer < diff)
        {
            //Cast
            // DoCastSpellIfCan(m_creature->getVictim(),SPELL_CSLUMBER);
            m_creature->CastSpell(m_creature->getVictim(), SPELL_CSLUMBER, true);

            //Stop attacking target thast asleep and pick new target
            Cslumber_Timer = 28000;

            Unit* Target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 0);

            if (!Target || Target == m_creature->getVictim())
                Target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0);

            if (Target)
                m_creature->TauntApply(Target);

        }
        else Cslumber_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mob_jadespine_basilisk(Creature* pCreature)
{
    return new mob_jadespine_basiliskAI(pCreature);
}

/*######
## npc_lore_keeper_of_norgannon
######*/

bool GossipHello_npc_lore_keeper_of_norgannon(Player* pPlayer, Creature* pCreature)
{
    if (pPlayer->GetQuestStatus(2278) == QUEST_STATUS_INCOMPLETE)
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Who are the Earthen?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

//    pPlayer->SEND_GOSSIP_MENU(1079, pCreature->GetGUID());

    pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetObjectGuid());

    return true;
}

bool GossipSelect_npc_lore_keeper_of_norgannon(Player* pPlayer, Creature* pCreature, uint32 uiSender, uint32 uiAction)
{
    switch (uiAction)
    {
        case GOSSIP_ACTION_INFO_DEF+1:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "What is a \"subterranean being matrix\"?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            pPlayer->SEND_GOSSIP_MENU(1080, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+2:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "What are the anomalies you speak of?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            pPlayer->SEND_GOSSIP_MENU(1081, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+3:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "What is a resilient foundation of construction?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
            pPlayer->SEND_GOSSIP_MENU(1082, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+4:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "So... the Earthen were made out of stone?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
            pPlayer->SEND_GOSSIP_MENU(1083, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+5:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Anything else I should know about the Earthen?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);
            pPlayer->SEND_GOSSIP_MENU(1084, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+6:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "I think I understand the Creators' design intent for the Earthen now. What are the Earthen's anomalies that you spoke of earlier?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 7);
            pPlayer->SEND_GOSSIP_MENU(1085, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+7:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "What high-stress environments would cause the Earthen to destabilize?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 8);
            pPlayer->SEND_GOSSIP_MENU(1086, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+8:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "What happens when the Earthen destabilize?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 9);
            pPlayer->SEND_GOSSIP_MENU(1087, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+9:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Troggs?! Are the troggs you mention the same as the ones in the world today?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 10);
            pPlayer->SEND_GOSSIP_MENU(1088, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+10:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "You mentioned two results when the Earthen destabilize. What is the second?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 11);
            pPlayer->SEND_GOSSIP_MENU(1089, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+11:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Dwarves!!! Now you're telling me that dwarves originally came from the Earthen?!", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 12);
            pPlayer->SEND_GOSSIP_MENU(1090, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+12:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "These dwarves are the same ones today, yes? Do the dwarves maintain any other links to the Earthen?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 13);
            pPlayer->SEND_GOSSIP_MENU(1091, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+13:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Who are the Creators?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 14);
            pPlayer->SEND_GOSSIP_MENU(1092, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+14:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "This is a lot to think about.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 15);
            pPlayer->SEND_GOSSIP_MENU(1093, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+15:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "I will access the discs now.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 16);
            pPlayer->SEND_GOSSIP_MENU(1094, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+16:
            pPlayer->CLOSE_GOSSIP_MENU();
            pPlayer->AreaExploredOrEventHappens(2278);
            break;
    }
    return true;
}

#define QUEST_HIDDEN_CHAMBER 2240

bool OnTrigger_at_map_chamber(Player* pPlayer, const AreaTriggerEntry *at)
{
    if (pPlayer->GetQuestStatus(QUEST_HIDDEN_CHAMBER) == QUEST_STATUS_INCOMPLETE)
        pPlayer->AreaExploredOrEventHappens(QUEST_HIDDEN_CHAMBER);
    return true;
}

enum
{
    SPELL_ULDAMAN_BOSS_OBJECT_VISUAL        = 11206,
    TIMER_KEEPERS_SUMMON                    = 3000,
};

class go_altar_of_the_keepersAI : public GameObjectAI
{
public:
    go_altar_of_the_keepersAI(GameObject* gobj) : GameObjectAI(gobj)
    {
        Reset();
    }
    bool _eventStarted;
    bool _eventFinished;
    uint32 _summonTimer;
    void Reset()
    {
        _eventStarted = false;
        _eventFinished = false;
        _summonTimer  = TIMER_KEEPERS_SUMMON;
    }
    void UpdateAI(const uint32 diff)
    {
        if (_eventFinished)
            return;
        if (!_eventStarted)
        {
            const Map::PlayerList& players = me->GetMap()->GetPlayers();
            uint32 count = 0;
            for (Map::PlayerList::const_iterator it = players.begin(); it != players.end(); ++it)
                if (Player* player = it->getSource())
                    if (Spell* spell = player->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
                        if (spell->m_spellInfo->Id == SPELL_ULDAMAN_BOSS_OBJECT_VISUAL)
                            ++count;
            if (count >= 3)
                _eventStarted = true;
            return;
        }
        if (_summonTimer <= diff)
        {
            _summonTimer = TIMER_KEEPERS_SUMMON;
            if (InstanceData* instance = me->GetInstanceData())
            {
                me->GetInstanceData()->SetData(DATA_STONE_KEEPERS, IN_PROGRESS);
                _eventFinished = true;
            }
        }
        else
            _summonTimer -= diff;
    }
};

GameObjectAI* GetAI_go_altar_of_the_keepers(GameObject* go)
{
    return new go_altar_of_the_keepersAI(go);
}

struct AnnoraAI : public ScriptedAI
{
    AnnoraAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_creature->SetVisibility(VISIBILITY_OFF);
        m_creature->setFaction(35);
        m_uiNbScorpion = 0;
        isSpawned = false;
        Reset();
    }

    uint32 m_uiNbScorpion;
    bool isSpawned;

    void Reset()
    {
    }

    void Aggro(Unit* pWho)
    {
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!isSpawned)
        {
            std::list<Creature*> m_EscortList;
            m_uiNbScorpion = 0;

            GetCreatureListWithEntryInGrid(m_EscortList, m_creature, 7078, 30.0f);
            for (std::list<Creature*>::iterator it = m_EscortList.begin(); it != m_EscortList.end(); ++it)
            {
                if ((*it)->isAlive())
                    m_uiNbScorpion++;
            }
            m_EscortList.clear();

            if (m_uiNbScorpion == 0)
            {
                m_creature->SetVisibility(VISIBILITY_ON);
                m_creature->GetMotionMaster()->MovePoint(1, -164.3657f, 210.7687f, -49.572f);
                isSpawned = true;
            }
        }


        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_annora(Creature* pCreature)
{
    return new AnnoraAI(pCreature);
}

enum
{
    SPELL_FIRE_SHIELD       =   2602,
    SPELL_FLAME_BUFFET      =   10452,

};

struct EarthenUldamanAI : public ScriptedAI
{
    EarthenUldamanAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiFireShield_Timer;
    uint32 m_uiFlame_Timer;

    void Reset()
    {
        m_uiFireShield_Timer      = 3000;
        m_uiFlame_Timer             = 6600;
    }

    void Aggro(Unit* pWho)
    {
        m_creature->SetInCombatWithZone();
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiFireShield_Timer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_FIRE_SHIELD, true) == CAST_OK)
                m_uiFireShield_Timer = 15000;
        }
        else
            m_uiFireShield_Timer -= uiDiff;

        if (m_uiFlame_Timer < uiDiff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_FLAME_BUFFET);
            m_uiFlame_Timer = 5700;
        }
        else
            m_uiFlame_Timer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_EarthenUldamanAI(Creature* pCreature)
{
    return new EarthenUldamanAI(pCreature);
}

void AddSC_uldaman()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "ulda_earthen";
    newscript->GetAI = &GetAI_EarthenUldamanAI;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_annora";
    newscript->GetAI = &GetAI_annora;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_jadespine_basilisk";
    newscript->GetAI = &GetAI_mob_jadespine_basilisk;
    newscript->RegisterSelf();


    newscript = new Script;
    newscript->Name = "mob_stone_keeper";
    newscript->GetAI = &GetAI_mob_stone_keeper;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_lore_keeper_of_norgannon";
    newscript->pGossipHello = &GossipHello_npc_lore_keeper_of_norgannon;
    newscript->pGossipSelect = &GossipSelect_npc_lore_keeper_of_norgannon;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "go_keystone_chamber";
    newscript->pGOHello = &GOHello_go_keystone_chamber;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "go_altar_of_the_keepers";
    newscript->GOGetAI = &GetAI_go_altar_of_the_keepers;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "at_map_chamber";
    newscript->pAreaTrigger = &OnTrigger_at_map_chamber;
    newscript->RegisterSelf();
}
