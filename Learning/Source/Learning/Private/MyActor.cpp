// Fill out your copyright notice in the Description page of Project Settings.

#include "MyActor.h"

AMyActor::AMyActor() {
    PrimaryActorTick.bCanEverTick = true;

    // 初始化静态网格体组件
    _mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
    SetRootComponent(_mesh);

    // 根据路径, 找到资源文件
    // 资源路径可以在内容浏览器中, 右击某一资源|复制引用
    auto FindTEXT = TEXT("StaticMesh '/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube'");
    static ConstructorHelpers::FObjectFinder<UStaticMesh> cubeAsset(FindTEXT);
    
    // 设置网格体属性
    if (cubeAsset.Succeeded()) {
        _mesh->SetStaticMesh(cubeAsset.Object);
        _mesh->SetRelativeLocation(FVector(0.0f));
    }
}

void AMyActor::BeginPlay() {
    Super::BeginPlay();
}

void AMyActor::Tick(float DeltaTime) {
    FRotator rotation = GetActorRotation();
    rotation.Yaw += DeltaTime * 20.0f;
    SetActorRotation(rotation);

    Super::Tick(DeltaTime);
}
