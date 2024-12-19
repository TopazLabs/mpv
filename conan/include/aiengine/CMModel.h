#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>

@interface CMModel : NSObject{
    MLModel *_model;
    NSURL *_compiledPath;
    MLPredictionOptions* _options;
    bool _postFix;
    bool _shouldAutorelease;
    int _version;
    int _deviceId;
}
@property(nonatomic, retain) NSURL* compiledPath;
@property(readonly) int modelVersion;
@property(readonly) bool postFix;

-(id)init;
-(bool)loadFromCompiledPath: (NSURL*) path toDevice:(int) deviceId;
-(bool)compileModel: (NSString*)filepath;
-(bool)isLoaded;
-(MLDictionaryFeatureProvider *)runModel:(NSMutableDictionary*)inputs;
 -(void)deleteModelCache: (NSString*)path;
-(void)printDescription;
-(int)getVersionForModel;
-(bool)dummyProc;
-(void)dealloc;


@end
