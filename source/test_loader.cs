// Private x86 integration host: actual installed 7H parser, option evaluator,
// IRO mapper, virtual file handlers and native DLL. Never starts/opens FF7.
// Only this test process's DLL import table is redirected to the real 7H APIs.
using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Xml;
class LoaderTest {
 [DllImport("kernel32",CharSet=CharSet.Unicode,SetLastError=true)] static extern IntPtr LoadLibraryW(string p);
 [DllImport("kernel32",CharSet=CharSet.Ansi,ExactSpelling=true)] static extern IntPtr GetProcAddress(IntPtr h,string p);
 [DllImport("kernel32")] static extern bool VirtualProtect(IntPtr p,UIntPtr length,uint protection,out uint old);
 [DllImport("kernel32",EntryPoint="CreateFileW",CharSet=CharSet.Unicode,SetLastError=true)] static extern IntPtr RealCreate(string p,uint access,uint share,IntPtr security,uint creation,uint flags,IntPtr template);
 [DllImport("kernel32",EntryPoint="ReadFile",SetLastError=true)] static extern int RealRead(IntPtr h,IntPtr bytes,uint length,IntPtr read,IntPtr overlapped);
 [DllImport("kernel32",EntryPoint="CloseHandle")] static extern int RealClose(IntPtr h);
 [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate int Selected(uint c);
 [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate IntPtr ClassName(uint c);
 [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate uint Owners(uint id);
 [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate int CanEquip(uint c,uint id);
 [UnmanagedFunctionPointer(CallingConvention.Cdecl,CharSet=CharSet.Unicode)] delegate int ReadSettings([MarshalAs(UnmanagedType.LPWStr)]string p);
 [UnmanagedFunctionPointer(CallingConvention.Winapi,CharSet=CharSet.Unicode)] delegate IntPtr CreateHook([MarshalAs(UnmanagedType.LPWStr)]string p,uint a,uint s,IntPtr security,uint creation,uint flags,IntPtr template);
 [UnmanagedFunctionPointer(CallingConvention.Winapi)] delegate int ReadHook(IntPtr h,IntPtr b,uint n,IntPtr read,IntPtr overlapped);
 [UnmanagedFunctionPointer(CallingConvention.Winapi)] delegate int CloseHook(IntPtr h);
 [UnmanagedFunctionPointer(CallingConvention.Winapi)] delegate uint ModuleHook(IntPtr module,IntPtr buffer,uint size);
 static CreateHook createHook=ForwardCreate;static ReadHook readHook=ForwardRead;static CloseHook closeHook=ForwardClose;
 static ModuleHook moduleHook=FixtureExecutable;
 static string program,fixture;static int count,virtualOpens,virtualReads;
 static Type wrap;static MethodInfo hCreate,hRead,hClose;static object info,archive,profileItem;
 static MethodInfo isConfigActive;static PropertyInfo settingsProperty;static Type settingType;
 static XmlNodeList options;static Selected selected;static ClassName className;static Owners owners;static CanEquip equip;
 static void Check(bool pass,string label){++count;if(!pass)throw new Exception(label);}
 static object Field(object v,string n){var t=v.GetType();var p=t.GetProperty(n);return p!=null?p.GetValue(v,null):t.GetField(n).GetValue(v);}
 static void Set(object v,string n,object value){v.GetType().GetProperty(n).SetValue(v,value,null);}
 static int Length(object v){int n=0;foreach(object o in (IEnumerable)v)++n;return n;}
 static object Construct(Type t,string path){var ctor=t.GetConstructors()[0];var pars=ctor.GetParameters();object[] vals=new object[pars.Length];for(int i=0;i<vals.Length;++i)vals[i]=i==0?(object)path:pars[i].HasDefaultValue?pars[i].DefaultValue:pars[i].ParameterType==typeof(bool)?(object)false:null;return ctor.Invoke(vals);}
 static Delegate Export(IntPtr dll,string name,Type type){IntPtr p=GetProcAddress(dll,name);Check(p!=IntPtr.Zero,"missing export "+name);return Marshal.GetDelegateForFunctionPointer(p,type);}
 static object EnumArg(ParameterInfo[] p,int i,uint value){return Enum.ToObject(p[i].ParameterType,value);}
 static IntPtr ForwardCreate(string p,uint a,uint s,IntPtr security,uint creation,uint flags,IntPtr template){
  var pars=hCreate.GetParameters();IntPtr h=(IntPtr)hCreate.Invoke(null,new object[]{p,EnumArg(pars,1,a),EnumArg(pars,2,s),security,EnumArg(pars,4,creation),EnumArg(pars,5,flags),template});
  if(h!=IntPtr.Zero){++virtualOpens;return h;}return RealCreate(p,a,s,security,creation,flags,template);
 }
 static int ForwardRead(IntPtr h,IntPtr b,uint n,IntPtr read,IntPtr overlapped){int v=(int)hRead.Invoke(null,new object[]{h,b,n,read,overlapped});if(v!=0){++virtualReads;return v;}return RealRead(h,b,n,read,overlapped);}
 static int ForwardClose(IntPtr h){hClose.Invoke(null,new object[]{h});return RealClose(h);}
 static uint FixtureExecutable(IntPtr module,IntPtr buffer,uint size){
  Check(module==IntPtr.Zero,"only the test executable path is substituted");
  string path=Path.Combine(fixture,"ff7_en.exe");Check(path.Length+1<size,"test module path fits");
  char[] chars=(path+"\0").ToCharArray();Marshal.Copy(chars,0,buffer,chars.Length);return (uint)path.Length;
 }
 static void PatchImport(IntPtr dll,string name,Delegate hook){
  int pe=Marshal.ReadInt32(dll,0x3c),rva=Marshal.ReadInt32(dll,pe+24+104);
  for(IntPtr d=IntPtr.Add(dll,rva);Marshal.ReadInt32(d,12)!=0;d=IntPtr.Add(d,20)){
   int original=Marshal.ReadInt32(d),first=Marshal.ReadInt32(d,16);
   for(int i=0;Marshal.ReadInt32(dll,original+i*4)!=0;++i){
    int thunk=Marshal.ReadInt32(dll,original+i*4);if(thunk<0)continue;
    if(Marshal.PtrToStringAnsi(IntPtr.Add(dll,thunk+2))!=name)continue;
    IntPtr at=IntPtr.Add(dll,first+i*4);uint old,ignored;
    Check(VirtualProtect(at,(UIntPtr)4,0x40,out old),"test-only IAT protect");
    Marshal.WriteIntPtr(at,Marshal.GetFunctionPointerForDelegate(hook));VirtualProtect(at,(UIntPtr)4,old,out ignored);return;
   }
  }throw new Exception("import not found "+name);
 }
 static void MapSelection(int[] choices){
  IList settings=(IList)Activator.CreateInstance(settingsProperty.PropertyType);
  for(int c=0;c<9;++c){object setting=Activator.CreateInstance(settingType);Set(setting,"ID",options[c].SelectSingleNode("ID").InnerText);Set(setting,"Value",choices[c]);settings.Add(setting);}
  settingsProperty.SetValue(profileItem,settings,null);
  var mapped=wrap.GetField("_mappedFiles",BindingFlags.Static|BindingFlags.NonPublic);
  mapped.SetValue(null,Activator.CreateInstance(mapped.FieldType));
  int active=0;
  foreach(object folder in (IEnumerable)Field(info,"ModFolders")){
   object aw=Field(folder,"ActiveWhen");
   Func<string,bool> test=delegate(string spec){return (bool)isConfigActive.Invoke(profileItem,new object[]{spec});};
   Func<string,bool> no=delegate(string s){return false;};
   var evaluate=aw.GetType().GetMethod("IsActive");
   bool enabled=(bool)evaluate.Invoke(aw,evaluate.GetParameters().Length==2?new object[]{test,no}:new object[]{test,no,no});
   if(enabled){++active;wrap.GetMethod("AddIROFilesToMappedFiles",BindingFlags.Static|BindingFlags.NonPublic).Invoke(null,new object[]{Field(folder,"Folder"),null,archive});}
  }
  Check(active==9,"exactly one active folder per character");
  Check(((IDictionary)mapped.GetValue(null)).Count==9,"nine isolated virtual filenames");
 }
 static void Verify(int[] choices,ReadSettings read){
  MapSelection(choices);int before=virtualOpens;Check(read(fixture)==1,"read selected jobs through 7H virtual archive");
  Check(virtualOpens-before==9,"all nine markers read virtually");
  for(uint c=0;c<9;++c){Check(selected(c)==choices[c],"native selection matches Configure Mod");Check(Marshal.PtrToStringAnsi(className(c))==options[(int)c].SelectNodes("Option")[choices[c]].Attributes["Name"].Value,"native/UI class names agree");}
  for(uint id=0;id<256;++id){uint mask=owners(id);Check((mask&~511u)==0,"valid nine-character owner mask");Check(owners(0xABCDEF00u|id)==mask,"AP independence");
   for(uint c=0;c<9;++c)Check(equip(c,0xABCDEF00u|id)==((id==255 || (mask&(1u<<(int)c))!=0)?1:0),"dynamic icons/equip agreement");}
  Check(((IDictionary)wrap.GetField("_varchives",BindingFlags.Static|BindingFlags.NonPublic).GetValue(null)).Count==0,"virtual handles cleaned up");
 }
 static int Main(string[] args){try{
  program=args[0];string iro=Path.GetFullPath(args[1]),manifest=Path.GetFullPath(args[2]);fixture=Path.GetFullPath(args[3]);
  Directory.CreateDirectory(fixture);Directory.SetCurrentDirectory(fixture);
  AppDomain.CurrentDomain.AssemblyResolve+=delegate(object s,ResolveEventArgs e){string p=Path.Combine(program,new AssemblyName(e.Name).Name+".dll");return File.Exists(p)?Assembly.LoadFrom(p):null;};
  Assembly wrapper=Assembly.LoadFrom(Path.Combine(program,"AppWrapper.dll"));Type rt=wrapper.GetType("AppWrapper.RuntimeMod");
  archive=Construct(wrapper.GetType("AppWrapper.IrosArc"),iro);info=Construct(wrapper.GetType("AppWrapper.ModInfo"),manifest);
  Check(Length(Field(info,"Options"))==10,"nine character dropdowns and an accessory reference");Check(Length(Field(info,"ModFolders"))==225,"225 class-choice folders");Check(Length(Field(info,"LoadAssemblies"))==0,"no managed bridge");
  var doc=new XmlDocument();doc.Load(manifest);options=doc.SelectNodes("/ModInfo/ConfigOption");
  int[] defaults=new int[9];string[] expected={"Mystic Knight","Gunner","Monk","White Mage","Blue Mage","Thief","Summoner","Black Mage","Dragoon"};
  for(int c=0;c<9;++c){
   defaults[c]=int.Parse(options[c].SelectSingleNode("Default").InnerText);
   Check(options[c].SelectNodes("Option").Count==25,"25 classes available to each character");
   Check(options[c].SelectNodes("Option")[defaults[c]].Attributes["Name"].Value==expected[c],"documented default");
   Check(options[c].SelectSingleNode("ID").InnerText.StartsWith("job_v070_"),"new IDs avoid stale selections");
  }
  // Actual RuntimeMod extraction path used by 7H for native LoadLibrary.
  object mod=Activator.CreateInstance(rt,new object[]{iro,Activator.CreateInstance(typeof(List<>).MakeGenericType(wrapper.GetType("AppWrapper.ConditionalFolder"))),new List<string>(),info});
  rt.GetField("_archive",BindingFlags.NonPublic|BindingFlags.Instance).SetValue(mod,archive);
  IntPtr native=IntPtr.Zero;foreach(string path in (IEnumerable)rt.GetMethod("GetLoadLibraries").Invoke(mod,null)){native=LoadLibraryW(path);Check(native!=IntPtr.Zero,"native loader failure "+Marshal.GetLastWin32Error());}
  selected=(Selected)Export(native,"jobs_selected",typeof(Selected));className=(ClassName)Export(native,"jobs_class_name",typeof(ClassName));
  owners=(Owners)Export(native,"cmk_owner_mask",typeof(Owners));equip=(CanEquip)Export(native,"cmk_can_equip",typeof(CanEquip));
  ReadSettings read=(ReadSettings)Export(native,"jobs_read_settings",typeof(ReadSettings));
  for(uint c=0;c<9;++c)Check(selected(c)==defaults[c],"native startup defaults");
  // Evaluate ActiveWhen with the installed AppCore's private option evaluator.
  Assembly core=Assembly.LoadFrom(Path.Combine(program,"AppCore.dll"));
  Type pi=core.GetType("Iros.Workshop.ProfileItem");profileItem=Activator.CreateInstance(pi);
  settingsProperty=pi.GetProperty("Settings");settingType=settingsProperty.PropertyType.GetGenericArguments()[0];
  isConfigActive=pi.GetMethod("IsConfigActive",BindingFlags.Instance|BindingFlags.NonPublic);
  wrap=wrapper.GetType("AppWrapper.Wrap");hCreate=wrap.GetMethod("HCreateFileW");hRead=wrap.GetMethod("HReadFile");hClose=wrap.GetMethod("HCloseHandle");
  Type rp=wrapper.GetType("AppWrapper.RuntimeProfile");object profile=Activator.CreateInstance(rp);
  Set(profile,"FF7Path",fixture);Set(profile,"MonitorPaths",new List<string>());Set(profile,"gameFiles",new string[0]);
  wrap.GetField("_profile",BindingFlags.Static|BindingFlags.NonPublic).SetValue(null,profile);
  wrap.GetField("_process",BindingFlags.Static|BindingFlags.NonPublic).SetValue(null,Process.GetCurrentProcess());
  PatchImport(native,"CreateFileW",createHook);PatchImport(native,"ReadFile",readHook);PatchImport(native,"CloseHandle",closeHook);
  PatchImport(native,"GetModuleFileNameW",moduleHook);
  // Real DllMain loaded with defaults before file hooks were installed above.
  // Its first permission query must now read current virtual class selections.
  int[] initial=(int[])defaults.Clone();initial[0]=0;initial[7]=7;
  MapSelection(initial);Check(owners(49)==0,"first permission query defers settings until 7H file hooks are ready");
  for(uint c=0;c<9;++c)Check(selected(c)==initial[c],"deferred startup uses Configure Mod selections");
  Verify(defaults,read);
  for(int j=0;j<25;++j){int[] all=new int[9];for(int c=0;c<9;++c)all[c]=j;Verify(all,read);}
  for(int j=0;j<25;++j){int[] mixed=new int[9];for(int c=0;c<9;++c)mixed[c]=(j+c*3)%25;Verify(mixed,read);}
  int[] monks=new int[9];for(int c=0;c<9;++c)monks[c]=7;Verify(monks,read);
  Check(owners(49)==0,"Fire unowned when no eligible class selected");for(uint c=0;c<9;++c)Check(equip(c,49)==0,"unowned Fire unusable by every character");
  Check(owners(13)==511,"all nine Monk Counter Attack icons");
  var accessoryOwners=(Owners)Export(native,"jobs_accessory_owners",typeof(Owners));var accessoryEquip=(CanEquip)Export(native,"jobs_accessory_allowed",typeof(CanEquip));
  for(uint id=0;id<32;++id){uint mask=accessoryOwners(id);Check(mask!=0 && (mask&~511u)==0,"accessory owner mask");for(uint c=0;c<9;++c)Check(accessoryEquip(c,id)==((mask&(1u<<(int)c))!=0?1:0),"accessory icons/equip unchanged");}
  Check(virtualReads==virtualOpens && virtualOpens==53*9,"every marker was read through actual 7H IRO handlers");
  Console.WriteLine("PASS "+count+" installed-7th-Heaven parser/configuration/virtual-file/native checks; "+virtualReads+" virtual marker reads.");return 0;
 }catch(Exception e){Console.WriteLine(e);return 1;}}
}
