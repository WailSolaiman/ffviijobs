// Private x86 integration host: installed 7H parser, option evaluator,
// IRO mapping, virtual files and HEXT discovery. Never opens FF7 or saves profiles.
using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Xml;
class LoaderTest {
 [DllImport("kernel32")] static extern IntPtr VirtualAlloc(IntPtr p,UIntPtr size,uint allocation,uint protection);
 [DllImport("kernel32")] static extern bool VirtualFree(IntPtr p,UIntPtr size,uint mode);
 [DllImport("kernel32")] static extern UIntPtr VirtualQuery(IntPtr p,IntPtr info,UIntPtr size);
 static string program,fixture;static int count,virtualOpens,virtualReads;
 static Type wrap;static MethodInfo hCreate,hRead,hClose;static object info,archive,profileItem;
 static MethodInfo isConfigActive;static PropertyInfo settingsProperty;static Type settingType;
 static XmlNodeList options;
 static void CheckHext(Assembly wrapper){
  byte[] bytes=(byte[])archive.GetType().GetMethod("GetBytes").Invoke(archive,new object[]{"hext\\FFVIIJobs.txt"});
  string[] lines=System.Text.Encoding.UTF8.GetString(bytes).Split('\n');
  IntPtr area=VirtualAlloc(IntPtr.Zero,(UIntPtr)0x3000,0x3000,4),mbi=Marshal.AllocHGlobal(28);Check(area!=IntPtr.Zero,"private HEXT allocation");
  try{
   var expected=new Dictionary<int,byte>();
   for(int i=1;i<lines.Length;i++){
    string line=lines[i].Trim();if(line.Length==0)continue;
    int split=line.IndexOf('=');bool write=split>=0;if(!write)split=line.IndexOf(':');
    long original=Convert.ToInt64(line.Substring(0,split).Trim(),16);int offset=original==0x67DB38?0:0x1000+(int)(original-0x7B5800);
    Check(offset>=0 && offset<0x3000,"only expected HEXT ranges");
    if(write){string spec=line.Substring(split+1).Trim();if(spec.Contains(":")){string[] pair=spec.Split(':');int length=Convert.ToInt32(pair[1],16);for(int j=0;j<length;j++)expected[offset+j]=Convert.ToByte(pair[0],16);}else{string[] parts=spec.Split(new[]{' '},StringSplitOptions.RemoveEmptyEntries);for(int j=0;j<parts.Length;j++)expected[offset+j]=Convert.ToByte(parts[j],16);}}
    lines[i]=(area.ToInt64()+offset).ToString("X")+line.Substring(split);
   }
   var patch=wrapper.GetType("AppWrapper.HexPatch").GetMethod("Apply",BindingFlags.Static|BindingFlags.Public,null,new[]{typeof(Stream)},null);
   using(var input=new MemoryStream(System.Text.Encoding.UTF8.GetBytes(string.Join("\n",lines))))patch.Invoke(null,new object[]{input});
   foreach(var pair in expected)Check(Marshal.ReadByte(area,pair.Key)==pair.Value,"installed parser applied exact HEXT bytes");
   foreach(int offset in new[]{0,0x1000}){Check(VirtualQuery(IntPtr.Add(area,offset),mbi,(UIntPtr)28)!=UIntPtr.Zero,"query patched page");Check((Marshal.ReadInt32(mbi,20)&0xF0)!=0,"entry and bootstrap pages executable after all writes");}
  }finally{Marshal.FreeHGlobal(mbi);VirtualFree(area,UIntPtr.Zero,0x8000);}
 }
 static void Check(bool pass,string label){++count;if(!pass)throw new Exception(label);}
 static object Field(object v,string n){var t=v.GetType();var p=t.GetProperty(n);return p!=null?p.GetValue(v,null):t.GetField(n).GetValue(v);}
 static void Set(object v,string n,object value){v.GetType().GetProperty(n).SetValue(v,value,null);}
 static int Length(object v){int n=0;foreach(object o in (IEnumerable)v)++n;return n;}
 static object Construct(Type t,string path){var ctor=t.GetConstructors()[0];var pars=ctor.GetParameters();object[] vals=new object[pars.Length];for(int i=0;i<vals.Length;++i)vals[i]=i==0?(object)path:pars[i].HasDefaultValue?pars[i].DefaultValue:pars[i].ParameterType==typeof(bool)?(object)false:null;return ctor.Invoke(vals);}
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
   var evaluate=aw==null?null:aw.GetType().GetMethod("IsActive");
   bool enabled=aw==null || (bool)evaluate.Invoke(aw,evaluate.GetParameters().Length==2?new object[]{test,no}:new object[]{test,no,no});
   if(enabled){++active;wrap.GetMethod("AddIROFilesToMappedFiles",BindingFlags.Static|BindingFlags.NonPublic).Invoke(null,new object[]{Field(folder,"Folder"),null,archive});}
  }
  Check(active==10,"exactly one active folder per character");
  Check(((IDictionary)mapped.GetValue(null)).Count==10,"nine markers and runtime mapped");
 }

 static byte[] ReadVirtual(string path){
  var pars=hCreate.GetParameters();
  IntPtr h=(IntPtr)hCreate.Invoke(null,new object[]{Path.Combine(fixture,path),Enum.ToObject(pars[1].ParameterType,0x80000000u),Enum.ToObject(pars[2].ParameterType,3),IntPtr.Zero,Enum.ToObject(pars[4].ParameterType,3),Enum.ToObject(pars[5].ParameterType,0x80),IntPtr.Zero});
  Check(h!=IntPtr.Zero,"virtual file open "+path);++virtualOpens;
  IntPtr b=Marshal.AllocHGlobal(100000),n=Marshal.AllocHGlobal(4);
  try{Marshal.WriteInt32(n,0);Check((int)hRead.Invoke(null,new object[]{h,b,(uint)100000,n,IntPtr.Zero})!=0,"virtual read");++virtualReads;byte[] data=new byte[Marshal.ReadInt32(n)];Marshal.Copy(b,data,0,data.Length);return data;}
  finally{hClose.Invoke(null,new object[]{h});Marshal.FreeHGlobal(b);Marshal.FreeHGlobal(n);}
 }
 static void Verify(int[] choices){
  MapSelection(choices);
  for(int c=0;c<9;++c){string id=options[c].SelectSingleNode("ID").InnerText.Substring("job_v070_".Length);string value=System.Text.Encoding.UTF8.GetString(ReadVirtual("ffvii-job-system-v070\\"+id+".class")).Trim();Check(value==options[c].SelectNodes("Option")[choices[c]].Attributes["Name"].Value.ToLowerInvariant().Replace(" / ","_").Replace(" ","_"),"class marker matches selection");}
  byte[] core=ReadVirtual("ffvii-jobs-v100.bin");Check(core.Length>32 && BitConverter.ToUInt32(core,0)==0x31584A46,"flat runtime mapped");
  Check(((IDictionary)wrap.GetField("_varchives",BindingFlags.Static|BindingFlags.NonPublic).GetValue(null)).Count==0,"virtual handles closed");
 }
 [STAThread]static int Main(string[] args){try{
  program=Path.GetFullPath(args[0]);string iro=Path.GetFullPath(args[1]),manifest=Path.GetFullPath(args[2]);fixture=Path.GetFullPath(args[3]);Directory.CreateDirectory(fixture);Directory.SetCurrentDirectory(fixture);
  AppDomain.CurrentDomain.AssemblyResolve+=delegate(object s,ResolveEventArgs e){string p=Path.Combine(program,new AssemblyName(e.Name).Name+".dll");return File.Exists(p)?Assembly.LoadFrom(p):null;};
  Assembly wrapper=Assembly.LoadFrom(Path.Combine(program,"AppWrapper.dll"));archive=Construct(wrapper.GetType("AppWrapper.IrosArc"),iro);info=Construct(wrapper.GetType("AppWrapper.ModInfo"),manifest);
  CheckHext(wrapper);
  Check(Length(Field(info,"Options"))==10,"configuration count");Check(Length(Field(info,"ModFolders"))==226,"class folders plus runtime");
  foreach(string name in new[]{"LoadLibraries","LoadPlugins","LoadAssemblies","LoadPrograms"})Check(Length(Field(info,name))==0,"no activation-warning declaration: "+name);
  foreach(string name in (IEnumerable)archive.GetType().GetMethod("AllFileNames").Invoke(archive,null))Check(!name.EndsWith(".dll",StringComparison.OrdinalIgnoreCase) && !name.EndsWith(".exe",StringComparison.OrdinalIgnoreCase),"no DLL or EXE payload");
  var doc=new XmlDocument();doc.Load(manifest);options=doc.SelectNodes("/ModInfo/ConfigOption");
  var rt=wrapper.GetType("AppWrapper.RuntimeMod");object mod=Activator.CreateInstance(rt,new object[]{iro,Activator.CreateInstance(typeof(List<>).MakeGenericType(wrapper.GetType("AppWrapper.ConditionalFolder"))),new List<string>(),info});rt.GetField("_archive",BindingFlags.NonPublic|BindingFlags.Instance).SetValue(mod,archive);
  int patches=0;foreach(string name in (IEnumerable)rt.GetMethod("GetPathOverrideNames").Invoke(mod,new object[]{"hext"})){Check(name=="FFVIIJobs.txt","HEXT entry discovered");foreach(object entry in (IEnumerable)rt.GetMethod("GetOverrides").Invoke(mod,new object[]{"hext\\"+name})){++patches;Check((int)Field(entry,"Size")>100,"HEXT patch has content");}}Check(patches==1,"one HEXT startup patch");
  Assembly coreAssembly=Assembly.LoadFrom(Path.Combine(program,"AppCore.dll"));Type pi=coreAssembly.GetType("Iros.Workshop.ProfileItem");profileItem=Activator.CreateInstance(pi);settingsProperty=pi.GetProperty("Settings");settingType=settingsProperty.PropertyType.GetGenericArguments()[0];isConfigActive=pi.GetMethod("IsConfigActive",BindingFlags.Instance|BindingFlags.NonPublic);
  wrap=wrapper.GetType("AppWrapper.Wrap");hCreate=wrap.GetMethod("HCreateFileW");hRead=wrap.GetMethod("HReadFile");hClose=wrap.GetMethod("HCloseHandle");object profile=Activator.CreateInstance(wrapper.GetType("AppWrapper.RuntimeProfile"));Set(profile,"FF7Path",fixture);Set(profile,"MonitorPaths",new List<string>());Set(profile,"gameFiles",new string[0]);wrap.GetField("_profile",BindingFlags.Static|BindingFlags.NonPublic).SetValue(null,profile);wrap.GetField("_process",BindingFlags.Static|BindingFlags.NonPublic).SetValue(null,Process.GetCurrentProcess());
  int[] defaults=new int[9];for(int c=0;c<9;++c)defaults[c]=int.Parse(options[c].SelectSingleNode("Default").InnerText);Verify(defaults);
  for(int j=0;j<25;++j){int[] same=new int[9],mixed=new int[9];for(int c=0;c<9;++c){same[c]=j;mixed[c]=(j+c*3)%25;}Verify(same);Verify(mixed);}
  Check(virtualOpens==510 && virtualReads==510,"all 51 configurations and runtime reads");
  Console.WriteLine("PASS "+count+" installed-7H HEXT/configuration/virtual-file checks; "+virtualReads+" virtual reads; warning declarations empty.");return 0;
 }catch(Exception e){Console.WriteLine(e);return 1;}}
}
