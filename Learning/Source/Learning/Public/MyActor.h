// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyActor.generated.h"

UCLASS()
class LEARNING_API AMyActor : public AActor {
    GENERATED_BODY()

public:
    AMyActor();

    // 静态网格体组件
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* _mesh;

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;
};
