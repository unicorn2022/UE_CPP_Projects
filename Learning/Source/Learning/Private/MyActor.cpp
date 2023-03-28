// Fill out your copyright notice in the Description page of Project Settings.

#include "MyActor.h"

AMyActor::AMyActor() {
    PrimaryActorTick.bCanEverTick = true;

    // ��ʼ����̬���������
    _mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
    SetRootComponent(_mesh);

    // ����·��, �ҵ���Դ�ļ�
    // ��Դ·�������������������, �һ�ĳһ��Դ|��������
    auto FindTEXT = TEXT("StaticMesh '/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube'");
    static ConstructorHelpers::FObjectFinder<UStaticMesh> cubeAsset(FindTEXT);
    
    // ��������������
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
