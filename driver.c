/********************************************************************
*  \Arquivo	driver.c
*
*  \Detalhes	Simple Linux device driver (Own Workqueue)
*
*  \Autor	Pedro Cassiano Coleone RA:793249
*
*  \Fonte	https://embetronicx.com/tutorials/linux/device-drivers/work-queue-in-linux-own-workqueue/
*
*********************************************************************/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include<linux/slab.h>                 //kmalloc()
#include<linux/uaccess.h>              //copy_to/from_user()
#include<linux/sysfs.h> 
#include<linux/kobject.h> 
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/workqueue.h>            // Necessário para as workqueues
#include <linux/err.h>
 
#define IRQ_NO 11
 
static struct workqueue_struct *own_workqueue;
 
static void workqueue_fn(struct work_struct *work); 
 
static DECLARE_WORK(work, workqueue_fn);


/*Função Workqueue*/
static void workqueue_fn(struct work_struct *work){
	printk(KERN_INFO "Executando a Funcao Workqueue\n");
	return;
} 

//Manipula interrupção para IRQ 11. 
static irqreturn_t irq_handler(int irq,void *dev_id){
	printk(KERN_INFO "IRQ Compartilhado: Ocorreu Interrupcao\n");
	
	// Alocando o Trabalho na Fila
	queue_work(own_workqueue, &work);
       
        return IRQ_HANDLED;
}
 
volatile int etx_value = 0;

dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;
struct kobject *kobj_ref;


// Prototipos das Funções
static int __init etx_driver_init(void);
static void __exit etx_driver_exit(void);
 
// Funções do Driver
static int etx_open(struct inode *inode, struct file *file);
static int etx_release(struct inode *inode, struct file *file);
static ssize_t etx_read(struct file *filp, 
                char __user *buf, size_t len,loff_t * off);
static ssize_t etx_write(struct file *filp, 
                const char *buf, size_t len, loff_t * off);
 
// Funções Sysfs
static ssize_t sysfs_show(struct kobject *kobj, 
                struct kobj_attribute *attr, char *buf);
static ssize_t sysfs_store(struct kobject *kobj, 
                struct kobj_attribute *attr,const char *buf, size_t count);
 
struct kobj_attribute etx_attr = __ATTR(etx_value, 0660, sysfs_show, sysfs_store);


// Estrutura para operação de arquivos
static struct file_operations fops ={
        .owner          = THIS_MODULE,
        .read           = etx_read,
        .write          = etx_write,
        .open           = etx_open,
        .release        = etx_release,
};

// Essa função é chamada quando lermos o arquivo syfs
static ssize_t sysfs_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
        printk(KERN_INFO "Sysfs - Ler!!!\n");
        return sprintf(buf, "%d", etx_value);
}

// Essa função será chamada quando escrevermos o arquivo syfs
static ssize_t sysfs_store(struct kobject *kobj, struct kobj_attribute *attr,const char *buf, size_t count){
        printk(KERN_INFO "Sysfs - Escrever!!!\n");
        sscanf(buf,"%d",&etx_value);
        return count;
}

// Esta função será chamada quando abrirmos o arquivo do dispositivo 
static int etx_open(struct inode *inode, struct file *file){
        printk(KERN_INFO "Arquivo do Dispositivo Aberto...!!!\n");
        return 0;
}

// Esta função será chamada quando fecharmos o arquivo do dispositivo
static int etx_release(struct inode *inode, struct file *file){
        printk(KERN_INFO "Arquivo do Dispositivo Fechado..!!!\n");
        return 0;
}

// Esta função será chamada quando lermos o arquivo do dispositivo
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off){
        printk(KERN_INFO "Funcao de Leitura\n");
        asm("int $0x3B");  // Correspondente ao irq 11
        return 0;
}

// Esta função será chamada quando lermos o arquivo do dispositivo
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off){
        printk(KERN_INFO "Funcao de Escrita\n");
        return 0;
}
 
// Função de inicialização do modulo 
static int __init etx_driver_init(void){
        // Alocando Major Number
        if((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) < 0){
                printk(KERN_INFO "Nao foi possivel alocar o major number\n");
                return -1;
        }
        printk(KERN_INFO "Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));
 
        // Criando um cdev structure
        cdev_init(&etx_cdev,&fops);
 
        // Adicionando o dispositivo de character ao sistema
        if((cdev_add(&etx_cdev,dev,1)) < 0){
            printk(KERN_INFO "Nao foi possivel adicionar o dispositivo ao sistema\n");
            goto r_class;
        }
 
        // Criando struct class
        if(IS_ERR(dev_class = class_create(THIS_MODULE,"etx_class"))){
            printk(KERN_INFO "Nao foi possivel criar a struct class\n");
            goto r_class;
        }
 
        // Criando Dispositivo
        if(IS_ERR(device_create(dev_class,NULL,dev,NULL,"etx_device"))){
            printk(KERN_INFO "Nao foi possivel criar o Dispositivo 1\n");
            goto r_device;
        }
 
        // Criando um diretório em /sys/kernel/
        kobj_ref = kobject_create_and_add("etx_sysfs",kernel_kobj);
 
        // Criando um arquivo sysfs para o etx_value
        if(sysfs_create_file(kobj_ref,&etx_attr.attr)){
                printk(KERN_INFO"Nao foi possivel criar o arquivo sysfs...\n");
                goto r_sysfs;
        }
        if (request_irq(IRQ_NO, irq_handler, IRQF_SHARED, "etx_device", (void *)(irq_handler))) {
            printk(KERN_INFO "my_device: nao pode registrar o IRQ \n");
                    goto irq;
        }
 
        // Criando workqueue
        own_workqueue = create_workqueue("own_wq");
        
        printk(KERN_INFO "Insercao do Driver do Dispositivo...Feito!!!\n");
        return 0;
 
irq:
        free_irq(IRQ_NO,(void *)(irq_handler));
 
r_sysfs:
        kobject_put(kobj_ref); 
        sysfs_remove_file(kernel_kobj, &etx_attr.attr);
 
r_device:
        class_destroy(dev_class);
r_class:
        unregister_chrdev_region(dev,1);
        cdev_del(&etx_cdev);
        return -1;
}


// Função de saída do modulo
static void __exit etx_driver_exit(void){
        // Deleta workqueue
        destroy_workqueue(own_workqueue);
        free_irq(IRQ_NO,(void *)(irq_handler));
        kobject_put(kobj_ref); 
        sysfs_remove_file(kernel_kobj, &etx_attr.attr);
        device_destroy(dev_class,dev);
        class_destroy(dev_class);
        cdev_del(&etx_cdev);
        unregister_chrdev_region(dev, 1);
        printk(KERN_INFO "Remocao do Driver do Dispositivo...Feito!!!\n");
}
 
module_init(etx_driver_init);
module_exit(etx_driver_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pedro Coleone - Fonte: EmbeTronicX <embetronicx@gmail.com>");
MODULE_DESCRIPTION("Simple Linux device driver (Own Workqueue)");
MODULE_VERSION("1.0");
