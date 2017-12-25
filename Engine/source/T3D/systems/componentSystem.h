#pragma once

template<typename T>
class ComponentSystem
{
public:
   static Vector<T> smInterfaceList;

   ComponentSystem()
   {
      
   }
   virtual ~ComponentSystem()
   {
      smInterfaceList.clear();
   }

   static T* GetNewInterface()
   {
      smInterfaceList.increment();

      return &smInterfaceList.last();
   }

   static void RemoveInterface(T* q)
   {
      smInterfaceList.erase(q);
   }
};

template<typename T> Vector<T> ComponentSystem<T>::smInterfaceList(0);
