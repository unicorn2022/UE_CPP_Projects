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
    // 输出当前时间
    void OutputTime();
    // 生成一个Actor
    void CreateActor();
};
