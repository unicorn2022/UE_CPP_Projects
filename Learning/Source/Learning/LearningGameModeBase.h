// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LearningGameModeBase.generated.h"


UCLASS()
class LEARNING_API ALearningGameModeBase : public AGameModeBase {
    GENERATED_BODY()
public:
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage);
    virtual void StartPlay();

private:
    // �����ǰʱ��
    void OutputTime();
    // ����һ��Actor
    void CreateActor();
};
