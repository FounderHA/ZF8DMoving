// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/**
 * Arquivo mestre de tags do projeto Zf.
 *
 * Inclua APENAS este arquivo em qualquer lugar que precise de tags.
 * Nunca inclua os arquivos de sistema individuais diretamente — use este.
 *
 * Exemplo:
 *   #include "ZfGameplayTags.h"
 *   ASC->HandleGameplayEvent(ZfProgressionTags::Event_Character_LevelUp, &Payload);
 *   SpecHandle.Data->SetSetByCallerMagnitude(ZfProgressionTags::Data_XP_Amount, 150.f);
 *
 * Para adicionar um novo sistema de tags:
 *   1. Crie Zf<Sistema>Tags.h e Zf<Sistema>Tags.cpp com o namespace Zf<Sistema>Tags
 *   2. Inclua o novo header abaixo
 */

// ── Sistemas ────────────────────────────────────────────────────────────────
#include "ZfProgressionTags.h"
#include "ZfMainAttributeTags.h"
#include "Inventory/ZfInventoryTags.h"