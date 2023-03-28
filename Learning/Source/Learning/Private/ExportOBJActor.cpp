// Fill out your copyright notice in the Description page of Project Settings.

#include "ExportOBJActor.h"
#include <EngineUtils.h>
#include <Engine/StaticMeshActor.h>
#include <StaticMeshDescription.h>
#include <StaticMeshAttributes.h>
#include <iostream>
#include <fstream>
#include "./Learning/lodepng.h"

DEFINE_LOG_CATEGORY_STATIC(LogExportOBJActor, All, All);

AExportOBJActor::AExportOBJActor() {
    PrimaryActorTick.bCanEverTick = false;
}

void AExportOBJActor::BeginPlay() {
    Super::BeginPlay();
    // 将UE中的模型导出为OBJ文件
     OutputMeshToOBJ();
}

// 将静态网格体导出为OBJ文件
void AExportOBJActor::OutputMeshToOBJ() {
    // 找到Table对应的Actor
    AStaticMeshActor* mesh = nullptr;
    for (TActorIterator<AStaticMeshActor> it(GetWorld()); it; ++it) {
        mesh = *it;
        if (mesh->GetName() == meshName) break;
    }
    if (!mesh) return;

    // 获取静态网格体数据
    UStaticMeshComponent* component = mesh->GetStaticMeshComponent();
    if (!mesh) return;
    UStaticMesh* meshData = component->GetStaticMesh();
    AnalyseStaticMesh(mesh, meshData);
}

// 获取静态网格体的数据
void AExportOBJActor::AnalyseStaticMesh(AStaticMeshActor* mesh, UStaticMesh* meshData) {
    if (!mesh || !meshData) return;

    // LOD层级数
    int numLOD = meshData->GetNumLODs();
    // 第1个LOD级别内的顶点和三角形面总数
    int numVertices = meshData->GetNumVertices(0);
    int numTriangles = meshData->GetNumTriangles(0);
    UE_LOG(LogExportOBJActor, Warning, TEXT("%s: LOD层级数: %d, 顶点数: %d, 三角形面数: %d"), *meshData->GetName(), numLOD, numVertices,
        numTriangles);

    // 获取顶点位置、顶点属性、三角形图元
    std::vector<FVector3f> outPositions, outNormals;
    std::vector<FVector2f> outUVs;
    GetTriangleDataFromMeshData(meshData, outPositions, outNormals, outUVs);
    // 导出材质
    FString mtlName;                      // 只考虑只有一个材质
    std::map<FString, FString> mtlFiles;  // 但是材质对应的纹理可能有多个, 将其路径保存在mtlFiles映射表里
    GetMTLFromMeshData(meshData, mtlName, mtlFiles);

    // 导出MTL, OBJ文件
    ExportToMTLFile(mtlName, mtlFiles);
    ExportToOBJFile(mtlName, outPositions, outNormals, outUVs);
}

// 从MeshData中获取三角形位置、法线、UV坐标等属性
void AExportOBJActor::GetTriangleDataFromMeshData(
    UStaticMesh* meshData, std::vector<FVector3f>& outPositions, std::vector<FVector3f>& outNormals, std::vector<FVector2f>& outUVs) {
    if (!meshData) return;
    UStaticMeshDescription* description = meshData->GetStaticMeshDescription(0);

    // 获取顶点数组，顶点实例属性数组，以及材质名称数组
    const TVertexAttributesRef<FVector3f>& positions = description->GetVertexPositions();
    const TAttributesSet<FVertexInstanceID>& attributes = description->VertexInstanceAttributes();
    TPolygonGroupAttributesConstRef<FName> materials = description->GetPolygonGroupMaterialSlotNames();

    // 遍历所有多边形组, 从中找到所有的多边形对象
    for (int i = 0; i < description->GetPolygonGroupCount(); i++) {
        TArray<FPolygonID> polygons;
        description->GetPolygonGroupPolygons(i, polygons);
        UE_LOG(LogExportOBJActor, Warning, TEXT("%d: PolygonCount = %d, Material = %s"), i, polygons.Num(), *materials[i].ToString());

        // 遍历所有的多边形对象, 从中获取所有的三角形图元(通常只有1个)
        for (FPolygonID pID : polygons) {
            TArray<FTriangleID> triangles;
            description->GetPolygonTriangles(pID, triangles);

            // 遍历所有的三角形图元, 从中获取每个三角形对应的顶点实例索引值
            for (FTriangleID tID : triangles) {
                TArray<FVertexInstanceID> instanceIDs;
                description->GetTriangleVertexInstances(tID, instanceIDs);

                // 通过三个顶点的具体索引，从之前的顶点位置数组positions、顶点实例属性数组attributes中获取对应的向量数据
                for (int t = 0; t < 3; t++) {
                    FVertexInstanceID instanceID = instanceIDs[t];
                    FVertexID vID = description->GetVertexInstanceVertex(instanceID);
                    FVector3f position = positions[vID];
                    FVector3f normal = attributes.GetAttribute<FVector3f>(instanceID, MeshAttribute::VertexInstance::Normal, 0);
                    FVector2f uv = attributes.GetAttribute<FVector2f>(instanceID, MeshAttribute::VertexInstance::TextureCoordinate, 0);

                    outPositions.push_back(position);
                    outNormals.push_back(normal);
                    outUVs.push_back(uv);
                }
            }
        }
    }
}

// 从MeshData中获取STL材质
void AExportOBJActor::GetMTLFromMeshData(UStaticMesh* meshData, FString& mtlName, std::map<FString, FString>& mtlFiles) {
    if (!meshData) return;
    UStaticMeshDescription* description = meshData->GetStaticMeshDescription(0);
    TPolygonGroupAttributesConstRef<FName> materials = description->GetPolygonGroupMaterialSlotNames();

    for (int i = 0; i < description->GetPolygonGroupCount(); i++) {
        int mtlIndex = meshData->GetMaterialIndex(materials[i]);
        UMaterialInterface* mtl = meshData->GetMaterial(mtlIndex);
        mtlName = mtl->GetName();

        TArray<UTexture*> textures, textures2;
        // 编辑器模式下: 获取BaseColor和Normal两个属性各自对应的工作流中包含的所有纹理对象
        mtl->GetTexturesInPropertyChain(EMaterialProperty::MP_BaseColor, textures, NULL, NULL);
        mtl->GetTexturesInPropertyChain(EMaterialProperty::MP_Normal, textures2, NULL, NULL);
        mtlFiles["map_Ka"] = ExportToPNGFile(textures);
        mtlFiles["bump"] = ExportToPNGFile(textures2);
        // 运行模式下: 获取当前材质用到的所有纹理对象
        // 其各参数的定义依次为：输出的纹理对象组，材质质量级别，是否输出全部材质质量，渲染层级别，是否输出全部渲染层级别
        // mtl->GetUsedTextures(textures, EMaterialQualityLevel::Num, false, ERHIFeatureLevel::Num, true);
        // mtlFiles["map_Ka"] = ExportToPNGFile(textures);
    }
}

// 获取纹理信息, 并通过loadpng库导出为png文件
FString AExportOBJActor::ExportToPNGFile(TArray<UTexture*> textures) {
    FString resultFileName;
    for (UTexture* tex : textures) {
        UTexture2D* tex2D = dynamic_cast<UTexture2D*>(tex);
        if (!tex2D) continue;  // 只处理2D纹理

        // 获取纹理的长、宽
        int w = tex2D->GetSizeX(), h = tex2D->GetSizeY();
        UE_LOG(LogExportOBJActor, Warning, TEXT("Texture %s: %d x %d"), *tex2D->GetFName().ToString(), w, h);

        // 获取图像的具体像素数据, 并且将它们保存到我们outImageData中
        std::vector<unsigned char> outImageData;

        // 获取只读锁可能会失败, 因此我们需要首先转换纹理对象为缺省状态
        TextureCompressionSettings prevCompression = tex2D->CompressionSettings;
        TextureMipGenSettings prevMipSettings = tex2D->MipGenSettings;
        uint8 prevSRGB = tex2D->SRGB;
        tex2D->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
        tex2D->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
        tex2D->SRGB = false;
        tex2D->UpdateResource();

        // 找到纹理对象Mipmaps第1层数据, 临时锁定当前的纹理对象
        // 获取到像素数据的内存地址, 进行读取处理之后, 再通过Unlock()结束锁定
        const FColor* imageData = static_cast<const FColor*>(tex2D->PlatformData->Mips[0].BulkData.LockReadOnly());
        if (imageData != NULL) {
            for (int y = 0; y < h; ++y)
                for (int x = 0; x < w; ++x) {
                    FColor color = imageData[y * w + x];
                    outImageData.push_back(color.R);
                    outImageData.push_back(color.G);
                    outImageData.push_back(color.B);
                    outImageData.push_back(color.A);
                }
        }
        tex2D->PlatformData->Mips[0].BulkData.Unlock();

        // 使用loadpng库导出文件
        FString filePath = filePathRoot + tex2D->GetFName().ToString();
        std::string fileName = TCHAR_TO_UTF8(*filePath);
        unsigned int result = lodepng::encode(fileName + ".png", outImageData, w, h);
        if (result > 0) {
            UE_LOG(LogExportOBJActor, Warning, TEXT("lodepng: %s"), UTF8_TO_TCHAR(lodepng_error_text(result)));
        } else {
            UE_LOG(LogExportOBJActor, Warning, TEXT("lodepng: %s, %d"), *tex2D->GetFName().ToString(), (int)outImageData.size());
        }
        resultFileName = UTF8_TO_TCHAR((fileName + ".png").c_str());

        // 将纹理参数设置回原状
        tex2D->CompressionSettings = prevCompression;
        tex2D->MipGenSettings = prevMipSettings;
        tex2D->SRGB = prevSRGB;
        tex2D->UpdateResource();
    }
    return resultFileName;
}

// 导出OBJ文件
void AExportOBJActor::ExportToOBJFile(FString mtlName, std::vector<FVector3f>& outPositions,
    std::vector<FVector3f>& outNormals, std::vector<FVector2f>& outUVs) {

    // 输出为OBJ文件
    FString filePath = filePathRoot + meshName + ".obj";
    std::ofstream out(*filePath);
    out << "# Export from UE5: " << TCHAR_TO_UTF8(*meshName) << std::endl;
    out << "mtllib " << TCHAR_TO_UTF8(*meshName) << ".mtl" << std::endl;
    out << "g " << TCHAR_TO_UTF8(*mtlName) << std::endl;
    out << "usemtl " << TCHAR_TO_UTF8(*mtlName) << std::endl;
    // 顶点位置
    for (size_t i = 0; i < outPositions.size(); ++i) {
        const FVector3f& v = outPositions[i];
        out << "v " << v[0] << " " << v[1] << " " << v[2] << std::endl;
    }
    // 顶点法线
    for (size_t i = 0; i < outNormals.size(); ++i) {
        const FVector3f& n = outNormals[i];
        out << "vn " << n[0] << " " << n[1] << " " << n[2] << std::endl;
    }
    // 顶点UV坐标
    for (size_t i = 0; i < outUVs.size(); ++i) {
        const FVector2f& uv = outUVs[i];
        out << "vt " << uv[0] << " " << uv[1] << std::endl;
    }
    // 三角面索引
    for (size_t i = 0; i < outPositions.size(); i += 3) {
        out << "f " << (i + 1) << "/" << (i + 1) << "/" << (i + 1) << " " << (i + 2) << "/" << (i + 2) << "/" << (i + 2) << " " << (i + 3)
            << "/" << (i + 3) << "/" << (i + 3) << " " << std::endl;
    }
    UE_LOG(LogExportOBJActor, Warning, TEXT("Output to OBJ file: Vertices = %d"), (int)outPositions.size());
}

// 导出MTL文件
void AExportOBJActor::ExportToMTLFile(FString& mtlName, std::map<FString, FString>& mtlFiles) {
    // 输出为mtl文件
    FString filePath = filePathRoot + meshName + ".mtl";
    std::ofstream out2(*filePath);
    out2 << "# Exported from UE5: " << TCHAR_TO_UTF8(*meshName) << std::endl;
    out2 << "newmtl " << TCHAR_TO_UTF8(*mtlName) << std::endl;
    out2 << "Ka 0.2 0.2 0.2" << std::endl;
    out2 << "Kd 0.6 0.6 0.6" << std::endl;
    out2 << "Ks 0.9 0.9 0.9" << std::endl;
    for (std::map<FString, FString>::iterator itr = mtlFiles.begin(); itr != mtlFiles.end(); ++itr) {
        out2 << TCHAR_TO_UTF8(*itr->first) << " " << TCHAR_TO_UTF8(*itr->second) << std::endl;
    }
}
