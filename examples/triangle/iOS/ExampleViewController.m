//
//  ExampleViewController.m
//  triangle
//
//  Created by Matthew Guerrette on 3/27/23.
//

#import "ExampleViewController.h"

@interface ExampleViewController ()
{
    GCVirtualController* _virtualController;
}
@end

@implementation ExampleViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    
    NSMutableSet<NSString*>* configButtons = [NSMutableSet new];
    [configButtons addObject:GCInputButtonA];
    [configButtons addObject:GCInputButtonX];
    [configButtons addObject:GCInputLeftThumbstick];
    GCVirtualControllerConfiguration* configuration = [GCVirtualControllerConfiguration new];
    configuration.elements = configButtons;
    
    _virtualController = [[GCVirtualController alloc] initWithConfiguration:configuration];
    [_virtualController connectWithReplyHandler:^(NSError * _Nullable error) {
        NSLog(@"%@", error);
    }];
    
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

@end
