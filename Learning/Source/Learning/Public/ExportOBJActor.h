// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <map>
#include <vector>
#include "ExportOBJActor.generated.h"

class AStaticMeshActor;
class UStaticMesh;

UCLASS()
class LEARNING_API AExportOBJActor : public AActor {
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, meta = (ToolTip = "待导出的Mesh在场景中的名称"))
    FString meshName = "Table";

    UPROPERTY(EditAnywhere, meta = (ToolTip = "导出路径"))
    FString filePathRoot = "D://Default//Desktop//OutputOBJ//";

    AExportOBJActor();

    // 将静态网格体导出为OBJ文件
    void ExportMeshToOBJ();

protected:
    virtual void BeginPlay() override;

private:
    UStaticMesh* meshData;
    std::vector<FVector3f> outPositions;    // 顶点位置
    std::vector<FVector3f> outNormals;      // 顶点法线
    std::vector<FVector2f> outUVs;          // 顶点UV坐标
    FString mtlName;                        // 只考虑只有一个材质
    std::map<FString, FString> mtlFiles;    // 但是材质对应的纹理可能有多个, 将其路径保存在mtlFiles映射表里

    // 获取静态网格体的数据
    void AnalyseStaticMesh();
    // 从MeshData中获取三角形位置、法线、UV坐标等属性
    void GetTriangleDataFromMeshData();
    // 从MeshData中获取STL材质
    void GetMTLFromMeshData();
    
    // 输出网格体的基础信息: LOD层级数、顶点数、三角形面数
    void LogBasicData();
    
    // 获取纹理信息, 并通过loadpng库导出为png文件
    FString ExportToPNGFile(TArray<UTexture*> textures);
    // 导出MTL文件
    void ExportToMTLFile();
    // 导出OBJ文件
    void ExportToOBJFile();
};
