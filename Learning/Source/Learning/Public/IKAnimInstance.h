// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "IKAnimInstance.generated.h"

UCLASS()
class LEARNING_API UIKAnimInstance : public UAnimInstance {
    GENERATED_BODY()

public:
    // 关键点影响的骨骼名称
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IK Bone", meta = (ToolTip = "关键点影响的骨骼名称"))
    TArray<FString> BoneNames;

    // 关键点的位置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IK Bone", meta = (ToolTip = "关键点的位置"))
    TArray<FVector> PointPositions;

private:
    // 根据设定的参数, 启用双骨骼IK
    UFUNCTION(BlueprintCallable, Category = "IK Bone")
    void ApplyBoneIK();
};
