/// Copyright 2021 Google Inc. All rights reserved.
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///    http://www.apache.org/licenses/LICENSE-2.0
///
///    Unless required by applicable law or agreed to in writing, software
///    distributed under the License is distributed on an "AS IS" BASIS,
///    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
///    See the License for the specific language governing permissions and
///    limitations under the License.

#import "Source/common/SNTXPCMetricServiceInterface.h"

@implementation SNTXPCMetricServiceInterface

+ (NSXPCInterface *)metricServiceInterface {
  NSXPCInterface *r = [NSXPCInterface interfaceWithProtocol:@protocol(SNTMetricServiceXPC)];

  [r setClasses:[NSSet setWithObjects:[NSDictionary class], [NSArray class], [NSNumber class],
                                      [NSString class], [NSDate class], nil]
      forSelector:@selector(exportForMonitoring:)
    argumentIndex:0
          ofReply:NO];

  return r;
}

+ (NSString *)serviceID {
  return @"com.google.santa.metricservice.rust";
}

+ (NSXPCConnection *)configuredConnection {
  NSXPCConnection *c = [[NSXPCConnection alloc] initWithMachServiceName:[self serviceID] options:0];
  c.remoteObjectInterface = [self metricServiceInterface];
  return c;
}

@end
