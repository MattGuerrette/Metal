//
//  ExampleViewController.m
//  Metal
//
//  Created by Matthew Guerrette on 3/27/23.
//

#import "ExampleViewController.h"

#import <MetalKit/MetalKit.h>

#import "../Example.h"

@interface ExampleViewController ()
{
    MTKView* _view;
    Example* _example;
}
@end

@implementation ExampleViewController

- (void)viewDidLoad {
    
    [super viewDidLoad];
    
    _view = (MTKView*)self.view;
    _view.device = MTLCreateSystemDefaultDevice();
    
    if(!_view.device)
    {
        NSLog(@"Metal is not supported on this device");
        self.view = [[NSView alloc] initWithFrame:self.view.frame];
        return;
    }
    
    _example = [[Example alloc] init];
    _view.delegate = _example;
}

@end
