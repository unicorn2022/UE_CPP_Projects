// Copyright Epic Games, Inc. All Rights Reserved.

#include "LearningGameModeBase.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include "./Learning/Public/MyActor.h"
#include "./Learning/Public/ExportOBJActor.h"
#include "./Learning/Public/ImportOBJActor.h"

void ALearningGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) {
    Super::InitGame(MapName, Options, ErrorMessage);
    
    OutputTime();
}

void ALearningGameModeBase::StartPlay() {
    // 创建一个Actor
    //CreateActor();

    Super::StartPlay();
}


// 输出当前时间
void ALearningGameModeBase::OutputTime() {
    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%F %T");

    UE_LOG(LogTemp, Warning, TEXT("当前时间: % s"), UTF8_TO_TCHAR(ss.str().c_str()));
}


// 生成一个Actor
void ALearningGameModeBase::CreateActor() {
    // 生成参数
    FActorSpawnParameters spawnParams;
    spawnParams.Name = "ExportActor";
    spawnParams.Owner = this;
    // 位置、旋转
    FVector position = FVector(0.0f, 0.0f, 0.0f);
    FRotator rotation = FRotator(00.0f, 0.0f, 0.0f);
    // 生成Actor
    AExportOBJActor* instance = GetWorld()->SpawnActor<AExportOBJActor>(position, rotation, spawnParams);
    // 修改Actor在大纲视图下的显示名称
    instance->SetActorLabel("ExportActor");

    // 导出OBJ文件
    instance->meshName = "Table";
    instance->ExportMeshToOBJ();

    UE_LOG(LogTemp, Warning, TEXT("新建Actor: %s"), *(instance->GetName()));
}

