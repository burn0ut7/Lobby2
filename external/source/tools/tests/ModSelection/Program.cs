using GCWorkbench.UI;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Threading;
using System.IO;
static class Checks {
 static IEnumerable<T> Find<T>(DependencyObject root) where T:DependencyObject {
  if(root is T value)yield return value;
  foreach(var child in LogicalTreeHelper.GetChildren(root).OfType<DependencyObject>())foreach(var result in Find<T>(child))yield return result;
 }
 [STAThread] static void Main(){
  var app=new Application{ShutdownMode=ShutdownMode.OnExplicitShutdown};
  SynchronizationContext.SetSynchronizationContext(new DispatcherSynchronizationContext());
  var spec=new DialogSpec{Title="Update mods",PrimaryAction="Update selected",Fields=[new(){Id="lobby",Label="GC Lobby",Kind="check",Enabled=false,Checked=true,Help="Up to date"},new(){Id="core",Label="GC Core",Kind="check",Checked=false,Help="Update available"}]};
  var window=new ToolWindow(spec,tools:true,observeStatus:false);
  var fileReview=new ToolWindow(new DialogSpec{Title="Submit mission",PrimaryAction="Submit",Section="Submit for approval",Fields=[
   new(){Id="title",Label="Mission name",Value="COOP Nightfall",Required=true},
   new(){Id="body",Label="Notes for the reviewer",Kind="multiline",Value="New mission and cleanup."},
   new(){Id="filesA",Label="Added (2)",Kind="filelist",Value="Missions/Nightfall.conf\nworlds/Nightfall.ent"},
   new(){Id="filesM",Label="Changed (1)",Kind="filelist",Value="addon.gproj"},
   new(){Id="filesD",Label="Deleted (2)",Kind="filelist",Value="Missions/OldMission.conf\nworlds/OldMission.ent"}
  ]},tools:true,observeStatus:false);
  var fileBoxes=Find<TextBox>(fileReview).Skip(2).ToArray();
  if(fileBoxes.Length!=3||fileBoxes.Any(t=>!t.IsReadOnly||t.MaxHeight!=240))throw new Exception("File groups must remain readable and read-only");
  if(fileReview.Width!=Math.Min(1180,SystemParameters.WorkArea.Width)||fileReview.Height!=Math.Min(800,SystemParameters.WorkArea.Height))throw new Exception("Default hub size incorrect");
  if(Environment.GetEnvironmentVariable("GC_TEST_REVIEW_SCREENSHOT") is {Length:>0} reviewSnapshot) {
   var surface=(FrameworkElement)fileReview.Content;
   surface.Measure(new Size(fileReview.Width,fileReview.Height));surface.Arrange(new Rect(0,0,fileReview.Width,fileReview.Height));surface.UpdateLayout();
   var bitmap=new System.Windows.Media.Imaging.RenderTargetBitmap((int)fileReview.Width,(int)fileReview.Height,96,96,System.Windows.Media.PixelFormats.Pbgra32);
   bitmap.Render(surface);var encoder=new System.Windows.Media.Imaging.PngBitmapEncoder();encoder.Frames.Add(System.Windows.Media.Imaging.BitmapFrame.Create(bitmap));
   using var stream=File.Create(reviewSnapshot);encoder.Save(stream);
  }
  var checks=Find<CheckBox>(window).ToArray();var update=Find<Button>(window).Single(b=>Equals(b.Content,"Update selected"));
  if(checks[0].IsEnabled||checks[0].IsChecked==true||checks[0].Opacity>=1||update.IsEnabled)throw new Exception("Current mod or empty action enabled");
  checks[1].IsChecked=true;if(!update.IsEnabled)throw new Exception("Available selected mod did not enable update");
  checks[1].IsChecked=false;if(update.IsEnabled)throw new Exception("Empty selection remained enabled");
  checks[0].IsChecked=true;checks[1].IsChecked=true;
  update.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  if(window.Result.Action!="update"||(bool)window.Result.Values["lobby"]||!(bool)window.Result.Values["core"])throw new Exception("Disabled mod leaked into dispatch");
  var navigationWindow=new ToolWindow(ToolPages.Submission(),tools:true,observeStatus:false);
  var sharedRefresh=Find<Button>(navigationWindow).Single(b=>Equals(b.Content,"Refresh"));
  if(Grid.GetRow(sharedRefresh)!=1||sharedRefresh.MinHeight<50)throw new Exception("Refresh is not the large sidebar bottom action");
  foreach(var title in new[]{"Dependencies","GitHub","Update mods","Submit mission"}) {
   Find<Button>(navigationWindow).Single(b=>Equals(b.Content,title)).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
   if(Find<Button>(navigationWindow).Any(b=>Equals(b.Content,"Close"))||!ReferenceEquals(sharedRefresh,Find<Button>(navigationWindow).Single(b=>Equals(b.Content,"Refresh"))))throw new Exception("Tool footer duplicated Close or Refresh");
  }
  Console.WriteLine("PASS: one persistent bottom-left Refresh across tools, no per-tool Close buttons.");
  if(Environment.GetEnvironmentVariable("GC_TEST_SCREENSHOT") is {Length:>0} snapshot) {
   var surface=(FrameworkElement)navigationWindow.Content;
   surface.Measure(new Size(880,560));surface.Arrange(new Rect(0,0,880,560));surface.UpdateLayout();
   var bitmap=new System.Windows.Media.Imaging.RenderTargetBitmap(880,560,96,96,System.Windows.Media.PixelFormats.Pbgra32);
   bitmap.Render(surface);var encoder=new System.Windows.Media.Imaging.PngBitmapEncoder();encoder.Frames.Add(System.Windows.Media.Imaging.BitmapFrame.Create(bitmap));
   using var file=File.Create(snapshot);encoder.Save(file);
  }
  var setup=new ToolWindow(new DialogSpec{Title="External dependencies",PrimaryAction="Install",SecondaryAction="Close"},tools:true,observeStatus:false);
  if(setup.Result.Action!="cancel")throw new Exception("Setup started without user action");
  var busy=(HashSet<string>)typeof(ToolWindow).GetField("checking",System.Reflection.BindingFlags.NonPublic|System.Reflection.BindingFlags.Instance)!.GetValue(setup)!;
  busy.Add("External dependencies");
  typeof(ToolWindow).GetMethod("Render",System.Reflection.BindingFlags.NonPublic|System.Reflection.BindingFlags.Instance|System.Reflection.BindingFlags.DeclaredOnly)!.Invoke(setup,[new DialogSpec{Title="External dependencies",PrimaryAction="Install"}]);
  var install=Find<Button>(setup).Single(b=>Equals(b.Content,"Install"));
  if(install.IsEnabled)throw new Exception("Install allowed while dependencies still unknown");
  typeof(ToolWindow).GetMethod("Render",System.Reflection.BindingFlags.NonPublic|System.Reflection.BindingFlags.Instance|System.Reflection.BindingFlags.DeclaredOnly)!.Invoke(setup,[ToolPages.Dependencies() with {Items=[new("GC Workbench Tools","","Not installed","warning"),new("Git for Windows","","Not installed","warning")]}]);
  install=Find<Button>(setup).Single(b=>Equals(b.Content,"Install"));
  if(!install.IsEnabled)throw new Exception("Missing dependencies cannot be installed");
  install.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  if(setup.Result.Action!="install")throw new Exception("Install action unavailable after notice");
  var hub=new ToolWindow(spec,tools:true);
  Dictionary<string,object>? dispatched=null;
  hub.OperationRunner=(_,values,_)=>{dispatched=values;return Task.FromResult(new OperationResult("Updated"));};
  const System.Reflection.BindingFlags flags=System.Reflection.BindingFlags.NonPublic|System.Reflection.BindingFlags.Instance|System.Reflection.BindingFlags.DeclaredOnly;
  var pages=(Dictionary<string,DialogSpec>)typeof(ToolWindow).GetField("statusPages",flags)!.GetValue(hub)!;
  var pending=(HashSet<string>)typeof(ToolWindow).GetField("checking",flags)!.GetValue(hub)!;
  var render=typeof(ToolWindow).GetMethod("Render",flags)!;
  pages["Update mods"]=spec;
  var timer=System.Diagnostics.Stopwatch.StartNew();
  Find<Button>(hub).Single(b=>Equals(b.Content,"Submit mission")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  Find<Button>(hub).Single(b=>Equals(b.Content,"Update mods")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  if(pending.Count!=0||Find<CheckBox>(hub).Count()!=2)throw new Exception("Navigation discarded cached results or restarted checks");
  Console.WriteLine($"Cached page round trip: {timer.Elapsed.TotalMilliseconds:F0} ms");
  pending.Add("Update mods");
  render.Invoke(hub,[spec]);
  var ready=Find<CheckBox>(hub).Last();ready.IsChecked=true;
  if(!Find<Button>(hub).Single(b=>Equals(b.Content,"Update selected")).IsEnabled)throw new Exception("Ready mod blocked by another pending check");
  typeof(ToolWindow).GetMethod("SaveDraft",flags)!.Invoke(hub,null);
  render.Invoke(hub,[spec]);
  if(Find<CheckBox>(hub).Last().IsChecked!=true)throw new Exception("Progress discarded user selection");
  Find<Button>(hub).Single(b=>Equals(b.Content,"Update selected")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  if(hub.Result.Action!="cancel"||dispatched is null||!(bool)dispatched["core"]||(bool)dispatched["lobby"])throw new Exception("Partial results dispatched unchecked or pending mods or closed hub");
  if(!Find<TextBlock>(hub).Any(t=>t.Text=="Updated")||Find<Button>(hub).Any(b=>Equals(b.Content,"Updated"))||!Find<Button>(hub).Any(b=>Equals(b.Content,"Update selected")))throw new Exception("Update status must be separate from action");
  Console.WriteLine("PASS: cached navigation, partial-result action, selection retained across progress.");
  var submission=new ToolWindow(ToolPages.Submission(),tools:true);
  var completed=new TaskCompletionSource<OperationResult>();
  var reviewShown=false;
  submission.OperationRunner=async (_,_,review)=>{
   var answer=await review(new DialogSpec{Title="Review conflict",PrimaryAction="Use my file",SecondaryAction="Cancel"});
   if(answer.Action!="confirm")throw new Exception("Review did not return confirmation");
   reviewShown=true;
   return await completed.Task;
  };
  Find<Button>(submission).Single(b=>Equals(b.Content,"Submit")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  if(!Find<Button>(submission).Any(b=>Equals(b.Content,"Use my file")))throw new Exception("Review not embedded in hub");
  if(Find<Button>(submission).Single(b=>Equals(b.Content,"Refresh")).IsEnabled)throw new Exception("Refresh enabled during review");
  if(!Find<Button>(submission).Any(b=>Equals(b.Content,"Cancel")))throw new Exception("Review lost its necessary cancel action");
  if(Find<Button>(submission).Single(b=>Equals(b.Content,"Update mods")).IsEnabled)throw new Exception("Navigation enabled during review");
  Find<Button>(submission).Single(b=>Equals(b.Content,"Use my file")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  Pump(()=>reviewShown);
  if(submission.Result.Action!="cancel")throw new Exception("Hub exited while workflow pending");
  completed.SetResult(new("Submitted","Awaiting approval","https://github.com/example/missions/pull/1"));
  Pump(()=>Find<TextBlock>(submission).Any(t=>t.Text=="Submitted"));
  if(!Find<Button>(submission).Any(b=>Equals(b.Content,"Submit"))||Find<Button>(submission).Any(b=>Equals(b.Content,"Submitted")))throw new Exception("Submission status replaced action");
  if(!Find<Button>(submission).Any(b=>Equals(b.Content,"View submission"))||submission.Result.Action!="cancel")throw new Exception("Submission result closed hub or lost link");
  Console.WriteLine("PASS: embedded conflict review, pending action, same-window submitted result and link.");
  var closedSubmission=new ToolWindow(ToolPages.Submission(),tools:true);
  var submitAttempts=0;
  closedSubmission.OperationRunner=(_,_,_)=>Task.FromResult(++submitAttempts==1 ? new OperationResult("Resubmit","Request closed.","https://github.com/example/missions/pull/1") : new OperationResult("Submitted","Awaiting approval"));
  Find<Button>(closedSubmission).Single(b=>Equals(b.Content,"Submit")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  var resubmit=Find<Button>(closedSubmission).Single(b=>Equals(b.Content,"Retry"));
  if(!resubmit.IsEnabled)throw new Exception("Closed submission cannot be retried");
  resubmit.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  if(submitAttempts!=2||!Find<Button>(closedSubmission).Any(b=>Equals(b.Content,"Submit"))||!Find<TextBlock>(closedSubmission).Any(t=>t.Text=="Submitted"))throw new Exception("Resubmit did not rerun workflow");
  var currentPage=ToolPages.Dependencies() with { Items=[new("GC Workbench Tools","","Up to date","ready"),new("Git for Windows","","Up to date","ready")] };
  var missingAndOld=new ToolWindow(currentPage with {Items=[new("GC Workbench Tools","","Installation incomplete","warning"),new("Git for Windows","","Update available","warning")]},tools:true,observeStatus:false);
  if(!Find<Button>(missingAndOld).Single(b=>Equals(b.Content,"Install")).IsEnabled||!Find<Button>(missingAndOld).Single(b=>Equals(b.Content,"Uninstall")).IsEnabled)throw new Exception("Missing must take priority over outdated");
  var missingManager=new ToolWindow(currentPage with {Items=[currentPage.Items[0],new("Git for Windows","","Git Credential Manager missing","warning")]},tools:true,observeStatus:false);
  if(!Find<Button>(missingManager).Single(b=>Equals(b.Content,"Install")).IsEnabled)throw new Exception("Missing credential component must say Install");
  var selectable=new ToolWindow(currentPage with {Items=[new("GC Workbench Tools","","Not installed","warning"),new("Git for Windows","","Update available","warning")]},tools:true);
  var selections=Find<CheckBox>(selectable).ToArray();
  if(selections.Length!=2||selections.Any(c=>c.IsChecked!=true))throw new Exception("Dependencies must start selected");
  selections[0].IsChecked=false;
  if(!Find<Button>(selectable).Single(b=>Equals(b.Content,"Update")).IsEnabled)throw new Exception("Unselected missing tools affected Git update");
  Find<CheckBox>(selectable).Last().IsChecked=false;
  if(Find<Button>(selectable).Single(b=>Equals(b.Content,"Update")).IsEnabled||Find<Button>(selectable).Single(b=>Equals(b.Content,"Uninstall")).IsEnabled)throw new Exception("Empty selection allowed an operation");
  Find<CheckBox>(selectable).Last().IsChecked=true;
  var selectionChecks=(HashSet<string>)typeof(ToolWindow).GetField("checking",flags)!.GetValue(selectable)!;
  selectionChecks.Add("External dependencies");
  Dictionary<string,object>? chosen=null;
  selectable.OperationRunner=(_,values,_)=>{chosen=values;return Task.FromResult(new OperationResult("Installed"));};
  Find<Button>(selectable).Single(b=>Equals(b.Content,"Update")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  if(chosen is null||(bool)chosen["tools"]||!(bool)chosen["git"])throw new Exception("Install ignored selection");
  selectable.OperationRunner=(_,values,_)=>{chosen=values;return Task.FromResult(new OperationResult("Uninstalled"));};
  Find<Button>(selectable).Single(b=>Equals(b.Content,"Uninstall")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  if(chosen is null||(bool)chosen["tools"]||!(bool)chosen["git"]||Find<CheckBox>(selectable).First().IsChecked!=false)throw new Exception("Uninstall or rerender lost selection");
  Console.WriteLine("PASS: both selected by default, scoped actions and labels, empty selection disabled, selections retained.");
  var noticePage=currentPage with {Items=[currentPage.Items[0],new("Git for Windows","","Update available","warning")]};
  if(ToolPages.DependencyNotice(currentPage)!=null||ToolPages.DependencyNotice(noticePage)!="External dependency updates available")throw new Exception("Startup update notice is incorrect");
  if(ToolPages.DependencyNotice(noticePage with {Items=[new("GC Workbench Tools","","Not installed","warning"),noticePage.Items[1]]})!="External dependencies missing or outdated")throw new Exception("Combined dependency notice is incorrect");
  if(ToolPages.DependencyNotice(currentPage with {Items=[new("Git for Windows","","Installed · update check unavailable","warning")]})!=null)throw new Exception("Unknown status reported an update");
  var noticeWindow=new ToolWindow(noticePage,tools:true,observeStatus:false);
  var notice=noticeWindow.ShowDependencyNoticeAsync(noticePage);
  if(!Find<TextBlock>(noticeWindow).Any(t=>t.Text=="External dependency updates available"))throw new Exception("Update notice was not displayed");
  Find<Button>(noticeWindow).Single(b=>Equals(b.Content,"OK")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  Pump(()=>notice.IsCompleted);
  if(!Find<Button>(noticeWindow).Any(b=>Equals(b.Content,"Update"))||noticeWindow.Result.Action!="cancel")throw new Exception("Notice did not return to dependencies without performing an action");
  var lateNoticeWindow=new ToolWindow(spec,tools:true,observeStatus:false);
  ((Dictionary<string,DialogSpec>)typeof(ToolWindow).GetField("statusPages",flags)!.GetValue(lateNoticeWindow)!)["Update mods"]=spec;
  Find<CheckBox>(lateNoticeWindow).Last().IsChecked=true;
  var lateNotice=lateNoticeWindow.ShowDependencyNoticeAsync(noticePage);
  Find<Button>(lateNoticeWindow).Single(b=>Equals(b.Content,"OK")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  Pump(()=>lateNotice.IsCompleted);
  Find<Button>(lateNoticeWindow).Single(b=>Equals(b.Content,"Update mods")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  if(Find<CheckBox>(lateNoticeWindow).Last().IsChecked!=true)throw new Exception("Late startup notice discarded mod selection");
  foreach(var tab in new[]{"Submit mission","Update mods","GitHub","Dependencies"}) {
   var dismissible=new ToolWindow(noticePage,tools:true,observeStatus:false);
   var dismissed=dismissible.ShowDependencyNoticeAsync(noticePage,"External dependencies not found");
   var target=Find<Button>(dismissible).Single(b=>Equals(b.Content,tab));
   if(!target.IsEnabled)throw new Exception("Notice disabled sidebar tab");
   target.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
   Pump(()=>dismissed.IsCompleted);
   var expected=tab=="Dependencies"?"External dependencies":tab;
   if(!Find<TextBlock>(dismissible).Any(t=>t.Text==expected)||Find<Button>(dismissible).Any(b=>Equals(b.Content,"OK")))throw new Exception("Dismissed notice overrode selected tab");
   dismissible.Close();
  }
  var closeNotice=new ToolWindow(noticePage,tools:true,observeStatus:false);
  bool noticeClosed=false;closeNotice.Closed+=(_,_)=>noticeClosed=true;
  var closingNotice=closeNotice.ShowDependencyNoticeAsync(noticePage,"External dependencies not found");
  closeNotice.Close();
  Pump(()=>closingNotice.IsCompleted);
  if(!noticeClosed)throw new Exception("Notice prevented window closure");
  Console.WriteLine("PASS: notice dismisses through every tab or window close, without redirecting navigation.");
  Console.WriteLine("PASS: missing/update/combined notices, unknown status suppression, OK returns to dependencies.");
  var currentDependencies=new ToolWindow(currentPage,tools:true);
  if(Find<Button>(currentDependencies).Single(b=>Equals(b.Content,"Update")).IsEnabled)throw new Exception("Current dependencies allow reinstall");
  var uninstall=Find<Button>(currentDependencies).Single(b=>Equals(b.Content,"Uninstall"));
  if(!uninstall.IsEnabled||DockPanel.GetDock(uninstall)!=Dock.Left)throw new Exception("Uninstall must be available at the left even when current");
  ((HashSet<string>)typeof(ToolWindow).GetField("checking",flags)!.GetValue(currentDependencies)!).Add("External dependencies");
  var uninstallDone=new TaskCompletionSource<OperationResult>();string? uninstallAction=null;
  currentDependencies.OperationRunner=(action,_,_)=>{uninstallAction=action;return uninstallDone.Task;};
  uninstall.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  if(uninstallAction!="uninstall"||Find<Button>(currentDependencies).Single(b=>Equals(b.Content,"Uninstall")).IsEnabled)throw new Exception("Uninstall dispatch or busy guard failed");
  uninstallDone.SetResult(new("Uninstall stopped","Fixture failure"));
  Pump(()=>Find<TextBlock>(currentDependencies).Any(t=>t.Text=="Uninstall stopped"));
  if(!Find<Button>(currentDependencies).Single(b=>Equals(b.Content,"Uninstall")).IsEnabled||Find<Button>(currentDependencies).Any(b=>Equals(b.Content,"Retry"))||currentDependencies.Result.Action!="cancel")throw new Exception("Uninstall failure changed the install action or closed the hub");
  if(Environment.GetEnvironmentVariable("GC_TEST_DEPENDENCIES_SCREENSHOT") is {Length:>0} dependencySnapshot) {
   render.Invoke(currentDependencies,[currentPage]);
   var surface=(FrameworkElement)currentDependencies.Content;
   surface.Measure(new Size(1180,800));surface.Arrange(new Rect(0,0,1180,800));surface.UpdateLayout();
   var bitmap=new System.Windows.Media.Imaging.RenderTargetBitmap(1180,800,96,96,System.Windows.Media.PixelFormats.Pbgra32);
   bitmap.Render(surface);var encoder=new System.Windows.Media.Imaging.PngBitmapEncoder();encoder.Frames.Add(System.Windows.Media.Imaging.BitmapFrame.Create(bitmap));
   using var file=File.Create(dependencySnapshot);encoder.Save(file);
  }
  Console.WriteLine("PASS: left uninstall available when current, real action dispatch, busy guard, failure recovery without closing.");
  var pendingInstall=new TaskCompletionSource<OperationResult>();
  Dictionary<string,object>? selectedDependencies=null;
  var mixedPage=currentPage with { Items=[currentPage.Items[0],new("Git for Windows","","Update available","warning")] };
  var mixedDependencies=new ToolWindow(mixedPage,tools:true);
  ((HashSet<string>)typeof(ToolWindow).GetField("checking",flags)!.GetValue(mixedDependencies)!).Add("External dependencies");
  mixedDependencies.OperationRunner=(_,values,_)=>{selectedDependencies=values;return pendingInstall.Task;};
  Find<Button>(mixedDependencies).Single(b=>Equals(b.Content,"Update")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  if(selectedDependencies is null||(bool)selectedDependencies["tools"]||!(bool)selectedDependencies["git"])throw new Exception("Current dependency was selected for update");
  mixedDependencies.UpdateDependencyProgress(new("git","Updating...","pending"));
  if(!Find<TextBlock>(mixedDependencies).Any(t=>t.Text=="Updating...")||!Find<TextBlock>(mixedDependencies).Any(t=>t.Text=="Up to date"))throw new Exception("Per-item updating status missing or overwrote current item");
  mixedDependencies.UpdateDependencyProgress(new("git","Up to date","ready"));
  pendingInstall.SetResult(new("Installed","Dependencies are up to date."));
  Pump(()=>Find<TextBlock>(mixedDependencies).Any(t=>t.Text=="Installed"));
  if(Find<Button>(mixedDependencies).Single(b=>Equals(b.Content,"Update")).IsEnabled)throw new Exception("Completed dependencies allow reinstall");
  Console.WriteLine("PASS: current dependencies disabled, only outdated dependency dispatched, per-item updating and completion.");
  var dependencies=new ToolWindow(ToolPages.Dependencies() with {Items=[new("GC Workbench Tools","","Not installed","warning"),new("Git for Windows","","Not installed","warning")]},tools:true);
  var dependencyChecks=(HashSet<string>)typeof(ToolWindow).GetField("checking",flags)!.GetValue(dependencies)!;
  dependencyChecks.Add("External dependencies"); // Keep this test independent of network status checks.
  var attempts=0;
  var retried=new TaskCompletionSource<OperationResult>();
  dependencies.OperationRunner=(_,_,_)=>++attempts==1 ? Task.FromResult(new OperationResult("Installation stopped","Fixture error")) : retried.Task;
  Find<Button>(dependencies).Single(b=>Equals(b.Content,"Install")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  var retry=Find<Button>(dependencies).Single(b=>Equals(b.Content,"Install"));
  var footer=(Panel)retry.Parent;
  if(footer.Children.Count!=1||Find<Button>(dependencies).Any(b=>Equals(b.Content,"Close")||Equals(b.Content,"Installation stopped")))throw new Exception("Failed dependencies must keep their install action");
  if(!Find<TextBlock>(dependencies).Any(t=>t.Text=="Fixture error"))throw new Exception("Retry lost error detail");
  retry.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  if(attempts!=2||Find<Button>(dependencies).Single(b=>Equals(b.Content,"Install")).IsEnabled||!Find<TextBlock>(dependencies).Any(t=>t.Text=="Updating..."))throw new Exception("Retry did not run installation once with disabled progress button");
  retried.SetResult(new("Installed","Restart Workbench"));
  Pump(()=>Find<TextBlock>(dependencies).Any(t=>t.Text=="Installed"));
  if(Find<TextBlock>(dependencies).Any(t=>t.Text=="Updating..."))throw new Exception("Progress remained after completion");
  if(dependencies.Result.Action!="cancel")throw new Exception("Retry closed tools");
  Console.WriteLine("PASS: stable Install action, separate error, real retry dispatch, disabled progress and persistent completion.");
  var fixture=Path.Combine(Path.GetTempPath(),"gc-hub-test-"+Guid.NewGuid()+".ps1");
  var reviewPath=Path.GetFullPath("external/source/tools/GC-Review.ps1").Replace("'","''");
  File.WriteAllText(fixture,". '"+reviewPath+"'\nif($env:GC_INSTALL_TOOLS -ne '0' -or $env:GC_INSTALL_GIT -ne '1'){throw 'Wrong dependency selection'}\nWrite-Output 'GC_PROGRESS:{\"Dependency\":\"git\",\"Status\":\"Updating...\",\"State\":\"pending\"}'\nStart-Sleep -Milliseconds 500\n$answer=Show-GcReview @{Title='Fixture review';PrimaryAction='Confirm'}\nif($answer.Action -ne 'confirm'){throw 'Wrong review answer'}\nSet-GcOutcome 'Submitted' 'Fixture confirmed' 'https://github.com/example/missions/pull/1'\n");
  var oldScript=Environment.GetEnvironmentVariable("GC_WORKBENCH_INSTALL_SCRIPT");
  try {
   Environment.SetEnvironmentVariable("GC_WORKBENCH_INSTALL_SCRIPT",fixture);
   var progressItems=new List<DependencyProgress>();
   var operation=ToolOperations.RunAsync("install",new(){["tools"]=false,["git"]=true},request=>Task.FromResult(new DialogResult("confirm",[])),new Progress<DependencyProgress>(item=>progressItems.Add(item)));
   Pump(()=>progressItems.Count==1);
   if(operation.IsCompleted)throw new Exception("Progress arrived only after completion");
   Pump(()=>operation.IsCompleted);
   var receipt=operation.GetAwaiter().GetResult();
   Pump(()=>progressItems.Count==1);
   if(progressItems[0].Dependency!="git"||progressItems[0].Status!="Updating...")throw new Exception("Live child progress missing");
   if(receipt.Status!="Submitted"||receipt.Detail!="Fixture confirmed")throw new Exception("Pipe workflow did not return confirmed result");
   File.WriteAllText(fixture,"param([switch]$Uninstall)\n. '"+reviewPath+"'\nif(!$Uninstall -or $env:GC_UNINSTALL_TOOLS -ne '0' -or $env:GC_UNINSTALL_GIT -ne '1'){throw 'Wrong uninstall selection'}\nSet-GcOutcome 'Uninstalled' 'Fixture only'\n");
   var removal=ToolOperations.RunAsync("uninstall",new(){["tools"]=false,["git"]=true},_=>throw new Exception("Unexpected review"));
   Pump(()=>removal.IsCompleted);
   if(removal.GetAwaiter().GetResult().Status!="Uninstalled")throw new Exception("Uninstall did not use the external installer");
   Console.WriteLine("PASS: real Windows PowerShell review pipe and operation outcome, without external mutations.");
  } finally {Environment.SetEnvironmentVariable("GC_WORKBENCH_INSTALL_SCRIPT",oldScript);File.Delete(fixture);}
  var cleanupFixture=Path.Combine(Path.GetTempPath(),"gc-cleanup-pipe-"+Guid.NewGuid().ToString("N"));
  Directory.CreateDirectory(Path.Combine(cleanupFixture,"local","GCWorkbench","versions","old"));
  File.WriteAllText(Path.Combine(cleanupFixture,"Common.ps1"),File.ReadAllText("external/source/installer/support/Common.ps1")+"\nfunction Remove-GcLegacyStartup {}\n");
  File.Copy("external/source/installer/support/LobbyTools.ps1",Path.Combine(cleanupFixture,"LobbyTools.ps1"));
  File.WriteAllText(Path.Combine(cleanupFixture,"Cleanup.ps1"),"param([switch]$Wait)\nStart-Sleep -Seconds 4\n");
  var cleanupLauncher=Path.Combine(cleanupFixture,"Run.ps1");
  File.WriteAllText(cleanupLauncher,"param([switch]$Uninstall)\n$env:LOCALAPPDATA=Join-Path $PSScriptRoot 'local'\n. (Join-Path $PSScriptRoot 'Common.ps1')\n$result=Uninstall-GcIntegration\n[IO.File]::WriteAllText($env:GC_TOOLS_OUTCOME,(@{Status='Uninstalled';Detail=$result}|ConvertTo-Json -Compress))\n");
  try {
   Environment.SetEnvironmentVariable("GC_WORKBENCH_INSTALL_SCRIPT",cleanupLauncher);
   var elapsed=System.Diagnostics.Stopwatch.StartNew();
   var cleanupOperation=ToolOperations.RunAsync("uninstall",new(){["tools"]=true,["git"]=false},_=>throw new Exception("Unexpected review"));
   Pump(()=>cleanupOperation.IsCompleted);
   Console.WriteLine($"Cleanup receipt: {cleanupOperation.GetAwaiter().GetResult()} in {elapsed.Elapsed.TotalSeconds:F2}s");
   if(cleanupOperation.GetAwaiter().GetResult().Status!="Uninstalled"||elapsed.Elapsed.TotalSeconds>=3.5)throw new Exception("Uninstall waited for the background cleanup output pipe");
   Console.WriteLine("PASS: real uninstall returns while its background cleanup worker is still running.");
  } finally {Environment.SetEnvironmentVariable("GC_WORKBENCH_INSTALL_SCRIPT",oldScript);Directory.Delete(cleanupFixture,true);}
  var workflowDirectory=Path.GetFullPath(Path.Combine(AppContext.BaseDirectory,"..","lobby2-tools"));
  var workflow=Path.Combine(workflowDirectory,"GC-ModWorkflow.ps1");
  if(File.Exists(workflow))throw new Exception("Dispatch fixture would overwrite a workflow");
  Directory.CreateDirectory(workflowDirectory);
  File.WriteAllText(workflow,"param([switch]$SubmitMission,[string]$SelectedMods,[string]$Lobby2,[string]$GlobalConflictsCore,[string]$GlobalConflictsUnits,[string]$GlobalConflictsMissions)\n. '"+reviewPath+"'\nif(!$SubmitMission -and $SelectedMods -ne 'GlobalConflictsCore'){throw 'Wrong dispatch'}\nSet-GcOutcome 'Confirmed' 'Git-only dispatch'\n");
  try {
   foreach(var action in new[]{"submit","update"}) {
    var dispatchedOperation=ToolOperations.RunAsync(action,new(){["core"]=true},_=>throw new Exception("Unexpected editor preparation"));
    Pump(()=>dispatchedOperation.IsCompleted);
    if(dispatchedOperation.GetAwaiter().GetResult().Status!="Confirmed")throw new Exception("Git-only dispatch failed");
   }
   Console.WriteLine("PASS: submit and update launch PowerShell directly without editor preparation.");
  } finally {File.Delete(workflow);}
  Console.WriteLine("PASS: current mods gray/unchecked/disabled, button follows eligible selection, disabled mods excluded from dispatch.");
 }
 static void Pump(Func<bool> done){
  var deadline=DateTime.UtcNow.AddSeconds(20);
  while(!done()){
   if(DateTime.UtcNow>deadline)throw new Exception("UI test timed out");
   var frame=new DispatcherFrame();
   Dispatcher.CurrentDispatcher.BeginInvoke(DispatcherPriority.Background,new Action(()=>frame.Continue=false));
   Dispatcher.PushFrame(frame);
   Thread.Sleep(5);
  }
 }
}
