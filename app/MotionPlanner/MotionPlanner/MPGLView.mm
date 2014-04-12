//
//  MPGLView.m
//  MotionPlanner
//
//  Created by John Visentin on 4/4/14.
//  Copyright (c) 2014 Brockenhaus Studio. All rights reserved.
//

#import "MPGLView.h"
#import "BHGL.h"
#import "MPReader.h"
#import "MPDijkstra3D.h"
#import <Carbon/Carbon.h>

const float kMPSceneMinScale    = 0.5f;
const float kMPSceneMaxScale    = 2.0f;
const float kMPSceneScaleFactor = 0.2f;

const float kMPObjectMotionIncrement = 0.02f;

@interface MPGLView ()

@property (nonatomic, assign) MP::Dijkstra3D *planner;
@property (nonatomic, assign) std::vector<MP::Transform3D> &plan;

@property (nonatomic, strong) NSMutableDictionary *movementAnimations;

- (IBAction)openFile:(NSMenuItem *)sender;

@end

@implementation MPGLView

@dynamic scene;

#pragma mark - public interface

- (void)awakeFromNib
{
    [super awakeFromNib];
    
    NSOpenGLPixelFormatAttribute attrs[] = {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFADepthSize, 24,
        // Must specify the 3.2 Core Profile to use OpenGL 3.2
        NSOpenGLPFAOpenGLProfile,
        NSOpenGLProfileVersion3_2Core,
        NSOpenGLPFASupersample,
        NSOpenGLPFASampleBuffers, 1,
        NSOpenGLPFASamples, 4,
        0
    };
    
    NSOpenGLPixelFormat *pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    
    if (!pf)
    {
        NSLog(@"No OpenGL pixel format");
    }
    
    NSOpenGLContext* context = [[NSOpenGLContext alloc] initWithFormat:pf shareContext:nil];
    
    [self setPixelFormat:pf];
    
    [self setOpenGLContext:context];
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)mouseDragged:(NSEvent *)theEvent
{
    CGFloat dx = [theEvent deltaX] / self.bounds.size.width;
    
    BHGLBasicAnimation *rotateY = [BHGLBasicAnimation rotateBy:GLKQuaternionMakeWithAngleAndAxis(M_PI*dx, 0.0f, 1.0f, 0.0f)];
    
    [self.scene.rootNode runAnimation:rotateY];
}

- (void)scrollWheel:(NSEvent *)theEvent
{
    CGFloat dx = [theEvent scrollingDeltaY];
    
    float scale = dx < 0 ? 1.0f - kMPSceneScaleFactor : 1.0f + kMPSceneScaleFactor;
    
    scale *= self.scene.rootNode.scale.x;
    scale = fmaxf(kMPSceneMinScale, fminf(scale, kMPSceneMaxScale));
    
    BHGLBasicAnimation *scaleAnim = [BHGLBasicAnimation scaleTo:GLKVector3Make(scale, scale, scale) withDuration:0.1];
    
    [self.scene.rootNode runAnimation:scaleAnim];
}

- (void)keyDown:(NSEvent *)theEvent
{    
    unsigned short keyCode = [theEvent keyCode];
    
    GLKVector3 dp = GLKVector3Make(0.0f, 0.0f, 0.0f);
    
    int key = 0;
    
    switch (keyCode)
    {
        case kVK_LeftArrow:
        case kVK_ANSI_A:
            key = kVK_LeftArrow;
            dp.x -= kMPObjectMotionIncrement;
            break;
            
        case kVK_RightArrow:
        case kVK_ANSI_D:
            key = kVK_RightArrow;
            dp.x += kMPObjectMotionIncrement;
            break;
            
        case kVK_DownArrow:
        case kVK_ANSI_S:
            key = kVK_DownArrow;
            dp.z += kMPObjectMotionIncrement;
            break;
            
        case kVK_UpArrow:
        case kVK_ANSI_W:
            key = kVK_UpArrow;
            dp.z -= kMPObjectMotionIncrement;
            break;
            
        case kVK_ANSI_V:
            key = kVK_ANSI_V;
            dp.y += kMPObjectMotionIncrement;
            break;
            
        case kVK_Space:
            key = kVK_Space;
            dp.y -= kMPObjectMotionIncrement;
            break;
            
        default:
            break;
    }
    
    dp = GLKQuaternionRotateVector3(GLKQuaternionInvert(self.scene.rootNode.rotation), dp);
    
    if (key && ![self.movementAnimations objectForKey:@(key)])
    {
        __weak BHGLNode *wnode = self.scene.activeObject;
        BHGLBasicAnimation *trans = [BHGLBasicAnimation runBlock:^{
            wnode.position = GLKVector3Add(wnode.position, dp);
        }];
        trans.repeats = YES;
        
        [self.scene.activeObject runAnimation:trans];
        
        [self.movementAnimations setObject:trans forKey:@(key)];
    }
}

- (void)keyUp:(NSEvent *)theEvent
{
    unsigned short keyCode = [theEvent keyCode];

    int key = 0;
    
    switch (keyCode)
    {
        case kVK_LeftArrow:
        case kVK_ANSI_A:
            key = kVK_LeftArrow;
            break;
            
        case kVK_RightArrow:
        case kVK_ANSI_D:
            key = kVK_RightArrow;
            break;
            
        case kVK_DownArrow:
        case kVK_ANSI_S:
            key = kVK_DownArrow;
            break;
            
        case kVK_UpArrow:
        case kVK_ANSI_W:
            key = kVK_UpArrow;
            break;
            
        case kVK_ANSI_V:
            key = kVK_ANSI_V;
            break;
            
        case kVK_Space:
            key = kVK_Space;
            break;
            
        case kVK_ANSI_P:
        {
            MP::Transform3D start(MPVec3Make(-1, 0, 0), MPVec3Make(1, 1, 1), MPQuaternionIdentity);
            MP::Transform3D goal(MPVec3Make(1, 0, 0), MPVec3Make(1, 1, 1), MPQuaternionIdentity);
            
            if(!self.planner->plan(start, goal, self.plan))
            {
                NSLog(@"plan failed :(");
            }
            else
            {
                size_t dataSize = 3* self.plan.size() * sizeof(GLfloat);
                GLfloat *vertices = (GLfloat *)malloc(dataSize);
                
                for (int i = 0; i < self.plan.size(); ++i)
                {
                    memcpy(&vertices[3*i], self.plan.at(i).getPosition().v, 3 * sizeof(GLfloat));
                }
                
                BHGLVertexType vType = BHGLVertexTypeCreate(1);
                vType.attribs[0] = BHGLVertexAttribPosition;
                vType.types[0] = GL_FLOAT;
                vType.lengths[0] = 3;
                vType.normalized[0] = GL_FALSE;
                vType.offsets[0] = (GLvoid *)0;
                vType.stride = 3 * sizeof(GLfloat);
                
                BHGLMesh *mesh = [[BHGLMesh alloc] initWithVertexData:(const GLvoid *)vertices vertexDataSize:dataSize vertexType:&vType];
                mesh.primitiveMode = GL_LINE_STRIP;
                
                BHGLVertexTypeFree(vType);
                
                BHGLModelNode *planModel = [[BHGLModelNode alloc] init];
                planModel.mesh = mesh;
                
                [self.scene addChild:planModel];
                
                BHGLBasicAnimation *anim = [BHGLBasicAnimation transformWithBlock:^(BHGLAnimatedObject *object, NSTimeInterval current, NSTimeInterval duration) {
                    float t = current / duration;
                    int i = (int)(t * (self.plan.size()-1));
                    
                    object.position = MPVec3ToGLKVector3(self.plan.at(i).getPosition());
                    object.rotation = MPQuaternionToGLKQuaternion(self.plan.at(i).getRotation());
                    object.scale = MPVec3ToGLKVector3(self.plan.at(i).getScale());
                    
                } duration:10.0];
                
                [self.scene.activeObject runAnimation:anim];
            }
        }
            
        default:
            break;
    }
    
    BHGLAnimation *anim = [self.movementAnimations objectForKey:@(key)];
    
    if (anim)
    {
        [self.scene.activeObject removeAnimation:anim];
        [self.movementAnimations removeObjectForKey:@(key)];
    }
}

- (void)prepareOpenGL
{
    [super prepareOpenGL];
        
    glEnable(GL_MULTISAMPLE);
    
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    
//    glEnable(GL_DEPTH_TEST);
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.6f, 0.8f, 1.0f, 0.0f);
}

#pragma mark - private interface

- (NSMutableDictionary *)movementAnimations
{
    if (!_movementAnimations)
    {
        _movementAnimations = [NSMutableDictionary dictionaryWithCapacity:6];
    }
    return _movementAnimations;
}

- (void)openFile:(NSMenuItem *)sender
{
    NSOpenPanel *openPanel = [NSOpenPanel openPanel];
    
    [openPanel setCanChooseFiles:YES];
    [openPanel setCanChooseDirectories:NO];
    [openPanel setAllowsMultipleSelection:NO];
    
    [openPanel beginWithCompletionHandler:^(NSInteger result) {
        if (result == NSFileHandlingPanelOKButton)
        {
            NSString *chosenFile = [[openPanel URL] path];
            
            std::string filePath = std::string([chosenFile cStringUsingEncoding:NSUTF8StringEncoding]);
            
            MP::Reader reader(filePath);
            MP::Environment3D *environment = reader.generateEnvironment3D();
            environment->setStepSize(0.1);
            
            self.scene = [[MPScene alloc] initWithEnvironment:environment];
            
            delete self.planner;
            self.planner = new MP::Dijkstra3D(environment);
        }
    }];
}

@end
