// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <map>
#include "ImportOBJActor.generated.h"

class FMeshDescriptionBuilder;

UCLASS()
class LEARNING_API AImportOBJActor : public AActor {
    GENERATED_BODY()

public:
    AImportOBJActor();

    UPROPERTY(EditAnywhere, meta = (ToolTip = "a mesh name in the scene"))
    FString meshName = "Mesh";

    UPROPERTY(EditAnywhere, meta = (ToolTip = "path root of the output file"))
    FString filePathRoot = "D://Default//Desktop//TestModels//";

    UPROPERTY(EditAnywhere, meta = (ToolTip = "path root of the output file"))
    FString objFileName = "Minimal_Default.obj";

    // ��̬���������
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* _mesh;

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

private:
    // ��ǰ���빹�����������Ҫ����
    struct GlobalData {
        FMeshDescriptionBuilder* builder;
        TArray<FVertexID> vertexIDs;
        FPolygonGroupID polygonGroup;
    };
    // Ҫ�����VertexInstance�Ķ�����������
    struct TriangleVertex {
        int vertexIndex;
        FVector instanceNormal;
        FVector2D instanceUV;
    };

    // ������
    std::map<FPolygonGroupID, std::string> _materialIdMap;
    // ����ʵ��
    std::map<std::string, UMaterialInstanceDynamic*> _materialMap;

private:
    // ��������������
    UStaticMesh* CreateMeshDataFromFile(const FString& baseDir, const FString& file);
    // ��������
    UTexture2D* CreateTexture(const FString& baseDir, const FString& file);
    // �򼸺����������������Ϣ
    void AddTriangleData(GlobalData& globalData, const TriangleVertex& v1, const TriangleVertex& v2, const TriangleVertex& v3);
};
